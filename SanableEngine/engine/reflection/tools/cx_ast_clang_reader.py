import cx_ast
import cx_ast_tooling
from source_discovery import *
import config
import timings
import clang.cindex
import argparse

from collections.abc import Iterator
import typing


class ClangParseContext(cx_ast_tooling.ASTParser):
    @staticmethod
    def argparser_add_defaults(parser: argparse.ArgumentParser):
        cx_ast_tooling.ASTParser.argparser_add_defaults(parser)

    def __init__(this, project:Project, args:argparse.Namespace, parser_version_hash):
        super().__init__(project, args, parser_version_hash)

    def configure(this):
        return super().configure()

    def ingest(this):
        timings.switchTask(timings.TASK_ID_WALK_AST_INTERNAL)

        # Do removal of outdated/removed symbols
        for source in this.diff.outdated+this.diff.removed:
            symbolsToRemove = [i for i in this.module.contents.values() if i.sourceFile == source]
            for i in symbolsToRemove:
                this.module.remove(i)

        # Set DI hook
        this.module.findExternal = this.__tryFindExternal

        # Do parsing of outdated/new symbols
        for source in this.diff.outdated+this.diff.new:
            config.logger.info(f"Parsing {source}")
            for cursor in ClangParseContext._getChildren(source.parse()):
                # Only capture what's in the current file
                cursorFilePath = cursor.location.file.name.replace(os.altsep, os.sep)
                try:
                    if source.path == cursorFilePath:
                        this.__ingestCursor(None, cursor)
                    else:
                        this.__ingestExternal(None, cursor)
                except:
                    config.logger.critical(f"While parsing file {source.path}")
                    raise
        
        this.module.linkAll()


    @staticmethod
    def _getChildren(cursor: Cursor) -> Iterator[Cursor]:
        """
        Wrapper that helps us time most Clang operations
        """
        return timings.timeScoped(lambda: cursor.get_children(), timings.TASK_ID_WALK_AST_EXTERNAL)


    def __isOurs(this, cursor: Cursor):
        cursorFilePath = cursor.location.file.name.replace(os.altsep, os.sep)
        return any((cursorFilePath == i.path for i in this.project.files))


    factory_t = typing.Callable[[cx_ast.SymbolPath, Cursor, cx_ast.ASTNode, cx_ast.Module, Project], cx_ast.ASTNode|None]
    factories:dict[CursorKind, factory_t] = dict()

    def __cursorPath(this, parent:cx_ast.ASTNode, cursor:Cursor) -> cx_ast.SymbolPath:
        try:
            if parent != None:
                # Nested symbol
                return parent.path+cursor.spelling
            elif cursor.semantic_parent.kind != CursorKind.TRANSLATION_UNIT:
                # Namespaced globals/statics defined outside their
                # container need to have their paths fixed
                return _make_FullyQualifiedPath(cursor)
            else:
                # Non-namespaced non-class-static global
                return cx_ast.SymbolPath()+cursor.spelling
        except:
            config.logger.critical(f"While detecting path for symbol {cursor.displayname} at {makeSourceLocation(cursor, this.project)}")
            raise        

    def __ingestCursor(this, parent:cx_ast.ASTNode, cursor:Cursor) -> cx_ast.ASTNode|None:
        # No need to check if it's ours: we're guaranteed it is, if a parent is
        kind = cursor.kind
        if kind in ClangParseContext.factories.keys():
            path = this.__cursorPath(parent, cursor)
            
            # Try to parse node
            try: result = ClangParseContext.factories[kind](path, cursor, parent, this.module, this.project)
            except:
                config.logger.critical(f"While ingesting symbol {path}")
                raise

            if result != None:
                # Recurse into children
                for clang_child in ClangParseContext._getChildren(cursor):
                    child = this.__ingestCursor(result, clang_child)
                    if child != None:
                        child.owner = result
                        result.children.append(child)
                        
                # TEMPFIX paths of callables so they include parameters
                if isinstance(result, cx_ast.Callable):
                    # Fix path
                    params = [i.typeName for i in result.parameters]
                    affixes = []
                    if isinstance(result, cx_ast.MemFuncInfo):
                        if result.isThisObjConst   : affixes.append("const")
                        if result.isThisObjVolatile: affixes.append("volatile")
                    ownPart = cx_ast.SymbolPath.CallParameterized(result.path.ownName, params, affixes)
                    result.path = (result.path.parent if result.path.parent != None else cx_ast.SymbolPath()) + ownPart
                    
                    # Fix child paths
                    #def _temp_fix_child_paths(tgt:cx_ast.ASTNode):
                    #    for i in tgt.children:
                    #        i.path = (i.owner.path if i.owner != None else cx_ast.SymbolPath()) + i.path.ownName
                    #        this.module.refreshPath(i)
                    #        _temp_fix_child_paths(i)
                    #_temp_fix_child_paths(result)
                
                try: this.module.register(result)
                except:
                    config.logger.critical(f"While registering symbol {path}")
                    raise

            return result
        else:
            config.logger.debug(f"Skipping symbol of unhandled kind {kind}")
    
    class LazyExternal:
        def __init__(this, cursor, parent, sourceLoc):
            this.cursor = cursor
            this.parent = parent
            this.sourceLoc = sourceLoc
            this.expanded = False
            this.astNode = None

    def __ingestExternal(this, parent:cx_ast.ASTNode, cursor:Cursor) -> cx_ast.ASTNode|None:
        path = this.__cursorPath(parent, cursor)
        sourceLoc = makeSourceLocation(cursor, this.project)
        if sourceLoc == None: return # Exclude system libs

        if path not in this.module.externals.keys():
            this.module.externals[path] = []
            alreadyExists = False
        else:
            alreadyExists = any(i.sourceLoc == sourceLoc for i in this.module.externals[path])
            
        if not alreadyExists:
            # Lazy traversal - save for later in case it's needed
            this.module.externals[path].append(ClangParseContext.LazyExternal(cursor, parent, sourceLoc))
            
    def __tryFindExternal(this, pathRequested:cx_ast.SymbolPath) -> list[cx_ast.ASTNode]:
        if pathRequested in this.module.externals.keys():
            matchingGroup = this.module.externals[pathRequested]
            if all(i.expanded for i in matchingGroup):
                # Trivial case: already expanded
                return [i.astNode for i in this.module.externals[pathRequested]]
        
        # Requested a symbol that doesn't exist yet
        def longestKnownPath():
            p = pathRequested
            while p != None and len(p) > 0:
                if p in this.module.externals.keys(): return p
                p = p.parent
            return None # Top-level NS
        longestKnownPath = longestKnownPath()
            
        # Check tail condition: all nodes already expanded, but given node was not found
        if longestKnownPath in this.module.externals.keys() and all(i.expanded for i in this.module.externals[longestKnownPath]): return None

        # Check panic condition: can't locate (probably in system libs)
        if longestKnownPath not in this.module.externals.keys():
            config.logger.warning(f"Could not locate external(?) symbol: {pathRequested}")
            return None

        # Expand each candidate
        for ext in this.module.externals[longestKnownPath]:
            ext.expanded = True
                
            # Try to parse node
            path = this.__cursorPath(ext.parent, ext.cursor)
            kind = ext.cursor.kind
            if kind in ClangParseContext.factories.keys():
                try: ext.astNode = ClangParseContext.factories[kind](path, ext.cursor, ext.parent, this.module, this.project)
                except:
                    config.logger.critical(f"While ingesting external symbol {path}")
                    raise

            # Register children for lazy lookup as well
            if ext.astNode != None:
                for i in ext.cursor.get_children():
                    this.__ingestExternal(ext.astNode, i)

        # Recurse
        return this.__tryFindExternal(pathRequested)
                

        
    
def ASTFactory(*cursorTypes:CursorKind):
    """
    Helper to auto-register factory functions
    """
    def registrar(func:ClangParseContext.factory_t):
        for i in cursorTypes:
            assert i not in ClangParseContext.factories.keys()
            ClangParseContext.factories[i] = func
        return func
    return registrar



def makeSourceLocation(cursor:Cursor, project:Project):
    clang_loc:SourceLocation = cursor.location # Clang
    sourceFile = project.getFile(clang_loc.file.name)
    if sourceFile != None:
        return cx_ast.SourceLocation(sourceFile, clang_loc.line, clang_loc.column)
    else:
        # System library - cannot directly reference
        return None

def makeVisibility(cursor:Cursor):
    return {
            clang.cindex.AccessSpecifier.PUBLIC   : cx_ast.Member.Visibility.Public,
            clang.cindex.AccessSpecifier.PROTECTED: cx_ast.Member.Visibility.Protected,
            clang.cindex.AccessSpecifier.PRIVATE  : cx_ast.Member.Visibility.Private,
            clang.cindex.AccessSpecifier.INVALID  : cx_ast.Member.Visibility.Public # TODO imply that it's public but only because it isn't nested?
        }[cursor.access_specifier]

def isExplicitVirtualMethod(cursor:Cursor):
    return cursor.is_virtual_method() or cursor.is_pure_virtual_method()

def isExplicitOverride(cursor:Cursor):
    return any([i.kind == CursorKind.CXX_OVERRIDE_ATTR for i in ClangParseContext._getChildren(cursor)])


@ASTFactory(CursorKind.NAMESPACE)
def factory_Namespace(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.Namespace(
        path,
        makeSourceLocation(cursor, project)
    )

@ASTFactory(
    CursorKind.CLASS_DECL, CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION, # TODO only if fully specialized, else defer to CLASS_TEMPLATE handler
	CursorKind.STRUCT_DECL, CursorKind.UNION_DECL,
    CursorKind.CLASS_TEMPLATE
)
def factory_StructInfo(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.StructInfo(
        path,
        makeSourceLocation(cursor, project),
        cursor.is_definition(),
        cursor.is_abstract_record()
    ) # TODO add visibility specifier

@ASTFactory(CursorKind.FIELD_DECL)
def factory_FieldInfo(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.FieldInfo(
        path,
        makeSourceLocation(cursor, project),
        makeVisibility(cursor),
        _make_FullyQualifiedPathPath(cursor.type)
    )

@ASTFactory(CursorKind.CXX_BASE_SPECIFIER)
def factory_ParentInfo(ownPath:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.ParentInfo(
        ownPath.parent,
        _make_FullyQualifiedPath(cursor),
        makeSourceLocation(cursor, project),
        makeVisibility(cursor),
        any([i.spelling == "virtual" for i in cursor.get_tokens()])
    )

@ASTFactory(CursorKind.FRIEND_DECL)
def factory_FriendInfo(ownPath:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.FriendInfo(
        ownPath.parent,
        _make_FullyQualifiedPath( next(ClangParseContext._getChildren(cursor)) ),
        makeSourceLocation(cursor, project),
        makeVisibility(cursor)
    )

@ASTFactory(CursorKind.CONSTRUCTOR)
def factory_ConstructorInfo(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.ConstructorInfo(
        path,
        makeSourceLocation(cursor, project),
        cursor.is_definition(),
        cursor.is_deleted_method(),
        False, # TODO inline support
        makeVisibility(cursor)
    )

@ASTFactory(CursorKind.DESTRUCTOR)
def factory_DestructorInfo(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.DestructorInfo(
        path,
        makeSourceLocation(cursor, project),
        cursor.is_definition(),
        makeVisibility(cursor),
        isExplicitVirtualMethod(cursor),
        isExplicitOverride(cursor),
        cursor.is_deleted_method(),
        False # TODO inline support
    )

@ASTFactory(CursorKind.CXX_METHOD, CursorKind.FUNCTION_DECL, CursorKind.FUNCTION_TEMPLATE)
def factory_FuncInfo_MemberOrStaticOrGlobal(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    # Deduce parent, whether inline or out-of-line
    parentIsType = isinstance(parent, cx_ast.StructInfo)
    if not parentIsType: parentIsType = path.parent in module.contents.keys() and isinstance(module.contents[path.parent], cx_ast.StructInfo)
    
    if parentIsType:
        if not cursor.is_static_method():
            # Nonstatic member function
            return cx_ast.MemFuncInfo(
                path,
                makeSourceLocation(cursor, project),
                cursor.is_definition(),
                makeVisibility(cursor),
                isExplicitVirtualMethod(cursor),
                isExplicitOverride(cursor),
                _make_FullyQualifiedPathPath(cursor.result_type),
                cursor.is_deleted_method(),
                False, # TODO inline support
                cursor.is_const_method(),
                False # TODO volatile support
            )
        else:
            # Static member function
            return cx_ast.StaticFuncInfo(
                path,
                makeSourceLocation(cursor, project),
                cursor.is_definition(),
                makeVisibility(cursor),
                _make_FullyQualifiedPathPath(cursor.result_type),
                cursor.is_deleted_method(),
                False # TODO inline support
            )
    else:
        # Nonmember static function
        return cx_ast.GlobalFuncInfo(
            path,
            makeSourceLocation(cursor, project),
            cursor.is_definition(),
            _make_FullyQualifiedPathPath(cursor.result_type),
            cursor.is_deleted_method(),
            False # TODO inline support
        )

@ASTFactory(CursorKind.PARM_DECL)
def factory_ParameterInfo(ownPath:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    #if isinstance(lexicalParent, cx_ast.Callable.Parameter): return None # Working around a very stupid bug
    return cx_ast.Callable.Parameter(
        ownPath,
        makeSourceLocation(cursor, project),
        len([i for i in parent.children if isinstance(i, cx_ast.Callable.Parameter)]),
        _make_FullyQualifiedPathPath(cursor.type)
    )

@ASTFactory(CursorKind.VAR_DECL)
def factory_VarInfo_GlobalOrStatic(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    # Deduce parent, whether inline or out-of-line
    parentIsType = isinstance(parent, cx_ast.StructInfo)
    if not parentIsType: parentIsType = path.parent in module.contents.keys() and isinstance(module.contents[path.parent], cx_ast.StructInfo)

    if parentIsType:
        return cx_ast.StaticVarInfo(
            path,
            makeSourceLocation(cursor, project),
            cursor.is_definition(),
            makeVisibility(cursor),
            _make_FullyQualifiedPathPath(cursor.type)
        )
    else:
        return cx_ast.GlobalVarInfo(
            path,
            makeSourceLocation(cursor, project),
            cursor.is_definition(),
            _make_FullyQualifiedPathPath(cursor.type)
        )

@ASTFactory(CursorKind.ANNOTATE_ATTR)
def factory_Annotation(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    return cx_ast.Annotation(
        path.parent,
        cursor.displayname,
        makeSourceLocation(cursor, project)
    )

@ASTFactory(
    CursorKind.TEMPLATE_TYPE_PARAMETER,
    CursorKind.TEMPLATE_NON_TYPE_PARAMETER,
    CursorKind.TEMPLATE_TEMPLATE_PARAMETER
)
def factory_TemplateParam(path:cx_ast.SymbolPath, cursor:Cursor, parent:cx_ast.ASTNode, module:cx_ast.Module, project:Project):
    sourceFileContent = project.getFile(cursor.location.file.name).contents.split("\n")
    relevantLines = sourceFileContent[cursor.extent.start.line-1:cursor.extent.end.line]
    relevantLines[-1] = relevantLines[-1][:cursor.extent.end.column-1] # Must chop off end first in case this is single-line
    relevantLines[0] = relevantLines[0][cursor.extent.start.column-1:]
    paramLiteralText = "\n".join(relevantLines) # FIXME this won't work in macros
    
    # TODO: default values will be a sub-cursor?
    # TODO if we're doing it this way we also need to normalize names
    defaultVal = paramLiteralText.split("=")[-1].strip() if "=" in paramLiteralText else None
    
    return cx_ast.TemplateParameter(
        path,
        makeSourceLocation(cursor, project),
        len([i for i in parent.children if isinstance(i, cx_ast.TemplateParameter)]),
        paramLiteralText.split(" ")[0], # First word will be template, class, int, etc
        defaultVal
    )



def _dropLeading(val:str, to_drop:list[str]):
    for i in to_drop:
        if val.startswith(i):
            return _dropLeading(val[len(i):].strip(), to_drop)
    return val

def _getTemplateParameter(target:Type, index:int) -> cx_ast.QualifiedPath|str:
    assert target.get_num_template_arguments() != 0, f"Symbol {target.spelling} is not a template"
    cursor:Cursor = target.get_declaration()
    
    try:
        paramKind = cursor.get_template_argument_kind(index)
    except ValueError:
        # ??? why the heck is this erroring?
        # Seems like it means the type in question is a template parameter from a parent?
        templateArgs = target.spelling.split("<")[1][:-1].split(",")
        return templateArgs[index].strip()
        
    if paramKind == clang.cindex.TemplateArgumentKind.TYPE:
        paramType = cursor.get_template_argument_type(index)
        return _make_FullyQualifiedPathPath(paramType)
    else:
        return str(cursor.get_template_argument_value(index))

    # TODO is index absolute, or only based on how many of that param kind precede it?
    #prev_kinds = [cursor.get_template_argument_kind(i) for i in range(index)]
    #typed_index = 

def _getTemplateParameters(target:Type) -> list[cx_ast.QualifiedPath|str]:
    if target.get_num_template_arguments() == -1: return None # Idiom for non-template
    if "<" not in target.spelling or ">" not in target.spelling: return None # Template, but only via typedef
    return [_getTemplateParameter(target, i) for i in range(target.get_num_template_arguments())]

def _make_FullyQualifiedPathPath(target:Type):
    qualifiers = []
    if target.is_const_qualified   (): qualifiers.append("const")
    if target.is_volatile_qualified(): qualifiers.append("volatile")
    if target.is_restrict_qualified(): qualifiers.append("restrict")
    
    if target.spelling in ["auto", "decltype(auto)"]:
        # Special case: Can't deduce auto, decltype(auto)
        return cx_ast.QualifiedPath(
            base=_dropLeading(target.spelling, qualifiers),
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.NONE
        )
    elif target.kind == TypeKind.POINTER:
        return cx_ast.QualifiedPath(
            base=_make_FullyQualifiedPathPath(target.get_pointee()),
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.POINTER
        )
    elif target.kind == TypeKind.MEMBERPOINTER:
        raise Exception("Not implemented")
        return cx_ast.QualifiedPath(
            base=_make_FullyQualifiedPathPath(target.get_class_type()),
            owner=None, # TODO implement!
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.MEM_FIELD_POINTER # TODO implement member field pointer vs member func pointer
        )
    elif target.kind in [TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE]:
        return cx_ast.QualifiedPath(
            base=_make_FullyQualifiedPathPath(target.get_pointee()),
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.REFERENCE # TODO move-reference support
        )
    elif target.get_declaration().kind == CursorKind.NO_DECL_FOUND:
        # Template parameter or fundamental literal type
        return cx_ast.QualifiedPath(
            base=_dropLeading(target.spelling, qualifiers),
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.NONE
        )
    elif target.kind in [TypeKind.ELABORATED, TypeKind.RECORD]:
        templateParams = _getTemplateParameters(target)
        # Plain old type, or template parameter
        return cx_ast.QualifiedPath(
            _make_FullyQualifiedPath(target.get_declaration()),
            qualifiers=qualifiers,
            pointer_spec=cx_ast.QualifiedPath.Spec.NONE,
            template_params=templateParams if templateParams!=None else []
        )
    else:
        assert False, f"Unknown type kind: {target.kind} {target.spelling}"

def _make_FullyQualifiedPath(cursor:Cursor):
    parts = []
    
    while type(cursor) != type(None) and cursor.kind != CursorKind.TRANSLATION_UNIT:
        # Strip leading C-style record type specifier
        ownName = cursor.spelling
        def stripexact(val: str, leading: str): return val[len(leading):] if val.startswith(leading) else val
        ownName = stripexact(ownName, "enum ")
        ownName = stripexact(ownName, "class ") # Since this is after "enum" it will also catch "enum class"
        ownName = stripexact(ownName, "struct ")
        
        # Reconstruct template parameters
        # TODO
        #templateParams = [i for i in ClangParseContext._getChildren(target) if i.kind in [CursorKind.TEMPLATE_TYPE_PARAMETER, CursorKind.TEMPLATE_NON_TYPE_PARAMETER, CursorKind.TEMPLATE_TEMPLATE_PARAMETER]]
        
        parts.insert(0, ownName)
        cursor = cursor.semantic_parent

    out = cx_ast.SymbolPath()
    for i in parts: out = out + i
    return out
    

def cvpUnwrapTypeName(name: str, unwrapCv=True, unwrapPointers=True, unwrapArrays=True) -> str:
    # Preprocess name
    name = name.strip()

    # Pointers
    if unwrapPointers:
        if name[-1] in "*&":
            return cvpUnwrapTypeName(name[:-1], unwrapCv=unwrapCv, unwrapPointers=unwrapPointers, unwrapArrays=unwrapArrays)

    # Arrays
    if unwrapArrays:
        if name.endswith("]"):
            return cvpUnwrapTypeName(name[:name.rfind("[")], unwrapCv=unwrapCv, unwrapPointers=unwrapPointers, unwrapArrays=unwrapArrays)

    # Outer const/volatile
    if unwrapCv:
        # const/volatile qualifiers (after name) - will only be present if already a ptr
        for symbolToStrip in ["const", "volatile"]:
            if name.endswith(symbolToStrip):
                charBeforeSymbol = name[-len(symbolToStrip)-1]
                if not (charBeforeSymbol.isalnum() or charBeforeSymbol == "_"):
                    return cvpUnwrapTypeName(name[:-len(symbolToStrip)], unwrapCv=unwrapCv, unwrapPointers=unwrapPointers, unwrapArrays=unwrapArrays)
            
        if name[-1] not in "*&":
            # const/volatile qualifiers (before name) - only if not already a ptr
            for symbolToStrip in ["const ", "volatile "]:
                if name.startswith(symbolToStrip):
                    return cvpUnwrapTypeName(name[len(symbolToStrip):], unwrapCv=unwrapCv, unwrapPointers=unwrapPointers, unwrapArrays=unwrapArrays)

    # Nothing to strip
    return name



if __name__ == "__main__":
    import sys

    timings.switchTask(timings.TASK_ID_INIT)
    arg_parser = argparse.ArgumentParser(
        prog=os.path.basename(__file__),
        description="STIX Clang reader: Emitting and caching ASTs as a build step"
    )
    ClangParseContext.argparser_add_defaults(arg_parser)
    args = arg_parser.parse_args(sys.argv[1:])

    config.logger.info("Discovering files")
    timings.switchTask(timings.TASK_ID_DISCOVER)
    cx_project = Project(
        cx_ast_tooling.reduce_lists(args.targets),
        cx_ast_tooling.reduce_lists(args.includes)
    )
    cx_parser = ClangParseContext(cx_project, args, config.batch_stable_hash(["cx_ast_clang_reader.py"]))
    
    config.logger.info("Loading cache")
    cx_parser.configure()
    cx_parser.loadPrevOutput()
    config.logger.user(str(cx_parser.diff))
    
    config.logger.user("Parsing")
    timings.switchTask(timings.TASK_ID_WALK_AST_INTERNAL)
    cx_parser.ingest()
    
    config.logger.info("Finalizing")
    timings.switchTask(timings.TASK_ID_FINALIZE)
    cx_parser.saveOutput()
