from functools import cached_property
from textwrap import indent
import cx_ast
import cx_ast_tooling
import timings
import argparse
import os.path
import config
import source_discovery
import typing


class RttiGenerator(cx_ast_tooling.ASTConsumer):
    @staticmethod
    def argparser_add_defaults(parser: argparse.ArgumentParser):
        cx_ast_tooling.ASTConsumer.argparser_add_defaults(parser)
        parser.add_argument("-o", "--output", default=None, help="Specify an output folder/file. If not specified, defaults to target/src/rtti.generated.cpp")
        parser.add_argument("--template-file", dest="template_file", default=None)
        parser.add_argument("--default-image-capture-backend", dest="default_image_capture_backend", choices=["cdo", "disassembly"], default="disassembly")
        parser.add_argument("--default-image-capture-status", dest="default_image_capture_status", choices=["enabled", "disabled"], default="enabled")
        
    def __init__(this, args:argparse.Namespace):
        super().__init__(args)
        
        # Parse args
        this.args_output:str = args.output
        this.args_template:str = args.template_file
        this.default_image_capture_backend:str = args.default_image_capture_backend
        this.default_image_capture_status :str = args.default_image_capture_status

    def __isDefinitionOurs(this, symbol:cx_ast.ASTNode|list[cx_ast.ASTNode]):
        if isinstance(symbol, cx_ast.ASTNode) and symbol.definitionLocation != None:
            return symbol.definitionLocation.file in this.input.project.files
        elif isinstance(symbol, list):
            return any((
                s.definitionLocation != None and s.definitionLocation.file in this.input.project.files
                for s in symbol
            )) # Strongly discouraged
        else:
            return False # Probably...

    def configure(this):
        super().configure()
        stableSymbols = this.input.module.contents.values()
        stableSymbols = [i for i in stableSymbols if this.__isDefinitionOurs(i)] # Filter: only our files
        stableSymbols.sort(key=lambda i: str(i.path if isinstance(i, cx_ast.ASTNode) else i[0].path))
        this.stableSymbols = stableSymbols

    def execute(this):
        config.logger.user(f"Rendering binders")

        with open(this.args_template) as templateFile: template = templateFile.read()
        
        _rendered = this.__renderBody()
        renderedPreDecls = "\n"  .join(_rendered[0])
        renderedBody     = "\n\n".join(_rendered[1])

        generated = template \
                    .replace("GENERATED_RTTI", indent(renderedBody, ' '*4)) \
                    .replace("PUBLIC_CAST_DECLS", renderedPreDecls) \
                    .replace("INCLUDE_DEPENDENCIES", this.__renderIncludes())
        
        config.logger.user(f"Writing to {this.args_output}")
        with open(this.args_output, "wt") as outputFile: outputFile.write(generated)

    def __renderBody(this):
        forwardDecls = []
        bodyDecls = []
        
        # Render symbols
        for sym in this.stableSymbols:
            if type(sym) in RttiGenerator.renderers.keys() and not isinstance(sym, cx_ast.Member):
                if this.__isDefinitionOurs(sym):
                    genFn = RttiGenerator.renderers[type(sym)]
                    generated = genFn(sym)
                    if generated != None:
                        forwardDecls.append(f"//Forward decls for {sym.path}\n"+generated[0])
                        bodyDecls   .append(generated[1])
                else:
                    bodyDecls.append(f"//Skipping forward-declared symbol missing definition: {sym.path if not isinstance(sym, list) else sym[0].path}")

        return (forwardDecls, bodyDecls)
    
    rendererOutput_t = tuple[str,str]|None
    renderer_t = typing.Callable[[cx_ast.ASTNode], rendererOutput_t]
    renderers:dict[typing.Type, renderer_t] = dict() # Only applies to global symbols
    memRenderers:dict[typing.Type, renderer_t] = dict()

    defaultImageCaptureStatus = "enabled"
    defaultImageCaptureBackend = "disassembly"

    def __renderIncludes(this):
        includes:list[source_discovery.SourceFile] = list()
        for ty in this.stableSymbols:
            if isinstance(ty, cx_ast.StructInfo) and ty.definitionLocation != None:
                includes.append(ty.definitionLocation.file)
            elif isinstance(ty, cx_ast.GlobalFuncInfo):
                includes.extend([i.file for i in ty.declarationLocations])  
        includes = list(set(includes))
        includes.sort(key=lambda i: i.path)
        
        def shortestRelPath(sourceFile:source_discovery.SourceFile):
            def makeRelPath(parentDir:str):
                parentDir = os.path.normpath(parentDir)
                if os.path.normpath(sourceFile.path).startswith(parentDir):
                    return os.path.relpath(sourceFile.path, parentDir)
                else:
                    return None

            candidates = [makeRelPath(i) for i in this.input.project.includeDirs]
            candidates = [i for i in candidates if i != None]
            
            return min(candidates, key=len) if len(candidates)>0 else None

        relFiles = [shortestRelPath(i) for i in includes]
        return "\n".join([ f'#include "{i}"' for i in relFiles if i != None ])
    
def RttiRenderer(*types:typing.Type):
    """
    Helper to auto-register renderer functions
    """
    def registrar(func:RttiGenerator.renderer_t):
        for i in types:
            assert i not in RttiGenerator.renderers.keys()
            RttiGenerator.renderers[i] = func
        return func
    return registrar

def MemRttiRenderer(*types:typing.Type):
    """
    Helper to auto-register renderer functions
    """
    def registrar(func:RttiGenerator.renderer_t):
        for i in types:
            assert i not in RttiGenerator.memRenderers.keys()
            RttiGenerator.memRenderers[i] = func
        return func
    return registrar


########################## AST node renderers ##########################

def _getAllParents(ty:cx_ast.StructInfo):
    out:list[cx_ast.ParentInfo] = []
    
    # Accumulate
    for i in ty.children:
        if isinstance(i, cx_ast.ParentInfo):
            out.append(i)
            out.extend(_getAllParents(i.parentType))

    # Deduplicate
    out = list(set(out))
    out.sort(key=lambda v: str(v.parentTypePath))
    
    return out

@RttiRenderer(cx_ast.StructInfo)
def render_type(ty:cx_ast.StructInfo):
    # Don't write template types (for now)
    if ty.path.isDynamicallyInstanced: return None

    preDecls :list[str] = []
    bodyDecls:list[str] = [f"TypeBuilder builder = TypeBuilder::create<{ty.path}>();"]

    # Render children
    for mem in ty.children:
        if type(mem) in RttiGenerator.memRenderers.keys():
            genFn = RttiGenerator.memRenderers[type(mem)]
            memGenerated = genFn(mem)
            if memGenerated != None:
                preDecls .append(memGenerated[0])
                bodyDecls.append(memGenerated[1])

    # Render implicit virtual parents
    for p in _getAllParents(ty):
        if p.explicitlyVirtual and not p.owner == ty:
            # Always render. We break it with C-style cast, which ignores visibility.
            bodyDecls.append(f"builder.addParent<{ty.path}, {p.parentTypePath}>({p.visibility}, {cx_ast.ParentInfo.Virtualness.VirtualInherited});")

    # Render CDO capture
    if ty.isAbstract:
        bodyDecls.append(f"//{ty.path} is abstract. Skipping class image capture.")
    elif not detectImageCaptureStatus(ty):
        bodyDecls.append(f"//Class image capture is disabled for {ty.path}")
    else:
        backend = detectImageCaptureBackend(ty)
        if backend in imageCaptureBackends.keys():
            bodyDecls.append(imageCaptureBackends[backend](ty))
        else:
            bodyDecls.append(f"#error Unknown image capture backend: {backend}")
        
    # Render finalizer
    bodyDecls.append("builder.registerType(registry);")
    
    return (
        "\n".join(preDecls),
        "\n".join([
            f"//{ty.path}",
            "{",
            indent("\n".join(bodyDecls), ' '*4),
            "}"
        ])
    )

@MemRttiRenderer(cx_ast.ParentInfo)
def render_parent(parent:cx_ast.ParentInfo):
    # Always render. We break it with C-style cast, which ignores visibility.
    virtualness = cx_ast.ParentInfo.Virtualness.VirtualExplicit if parent.explicitlyVirtual else cx_ast.ParentInfo.Virtualness.NonVirtual
    body = f"builder.addParent<{parent.owner.path}, {parent.parentTypePath}>({parent.visibility}, {virtualness});"
    return ("", body)

def makePubCastKey(obj:cx_ast.ASTNode):
    return "".join([
        (i if i.isalnum() or i in "_" else '_')
        for i in str(obj.path)
    ])

@MemRttiRenderer(cx_ast.FieldInfo)
def render_field(field:cx_ast.FieldInfo):
    # Detect how to reference
    #if field.visibility != cx_ast.Member.Visibility.Public:
    pubCastKey = makePubCastKey(field)
    preDecl = f'PUBLIC_CAST_GIVE_FIELD_ACCESS({pubCastKey}, {field.path.parent}, {field.path.ownName}, {field.typeName});'
    pubReference = f"DO_PUBLIC_CAST_OFFSETOF_LAMBDA({pubCastKey}, {field.owner.path})"
    #else:
    #    preDecl = f"//{field.path} is already public: no need for public_cast"
    #    pubReference = f"[]({field.astParent.path}*)" + "{ " + f"return offsetof({field.path});" + " }"

    # TODO handle anonymous types (especially if private)
    body = f'builder.addField<{field.typeName}>("{field.path.ownName}", {pubReference});'
    
    return (preDecl, body)

@MemRttiRenderer(cx_ast.ConstructorInfo)
def render_constructor(ctor:cx_ast.ConstructorInfo):
    # Don't write template callables
    if ctor.path.isDynamicallyInstanced: return None

    if ctor.owner.isAbstract: return ("", f"//Skipping abstract constructor {ctor.path}")
    if ctor.deleted: return ("", f"//Skipping deleted constructor {ctor.path}")

    #paramNames = [i.displayName for i in ctor.parameters] # TODO implement name capture
    paramTypes = ", ".join([str(i.typeName) for i in ctor.parameters]) # Can't rely on template arg deduction in case of overloading
    
    thunkUtilsInstance = f"thunk_utils<{ctor.owner.path}>"
    ctorThunkInstance = thunkUtilsInstance+f"::thunk_newInPlace<{paramTypes}>"
    
    if ctor.visibility == cx_ast.Member.Visibility.Public or ctor.owner.isFriended(lambda f: thunkUtilsInstance in f.targetName):
        return ("", f"builder.addConstructor(stix::StaticFunction::make(&{ctorThunkInstance}), {ctor.visibility});")
    else:
        return ("", f"//Skipping inaccessible constructor {ctor.path}")

@MemRttiRenderer(cx_ast.MemFuncInfo)
def render_memFunc(func:cx_ast.MemFuncInfo):
    # Don't write template callables
    if func.path.isDynamicallyInstanced: return None

    # Detect how to reference
    #if func.visibility != cx_ast.Member.Visibility.Public:
    pubCastKey = makePubCastKey(func)
    formatter = {
        "key": pubCastKey,
        "TClass": func.path.parent,
        "returnType": func.returnTypeName,
        "params": ", ".join([str(i.typeName) for i in func.parameters]),
        "name": func.path.ownName.base,
        "this_qualifiers": (" const" if func.isThisObjConst else "") + (" volatile" if func.isThisObjVolatile else "")
    }
    preDecl = "\n".join([
        'PUBLIC_CAST_DECLARE_KEY_BARE({key});'.format_map(formatter),
		'template<> struct ::public_cast::_type_lut<PUBLIC_CAST_KEY_OF({key})>'.format_map(formatter) + ' { ' + 'using ptr_t = {returnType} ({TClass}::*)({params}){this_qualifiers};'.format_map(formatter) + ' };',
		'PUBLIC_CAST_GIVE_ACCESS_BARE({key}, {TClass}, {name});'.format_map(formatter)
    ])
    pubReference = f"DO_PUBLIC_CAST({pubCastKey})"
    #else:
    #    preDecl = f"//{func.path} is already public: no need for public_cast"
    #    pubReference = func.path
    
    if not func.deleted:
        paramNames = [i.path.ownName for i in func.parameters] # TODO implement name capture on C++ side
        body = f"builder.addMemberFunction(stix::MemberFunction::make({pubReference}), \"{func.path.ownName.base}\", {func.visibility}, {str(func.isVirtual).lower()});"
    else:
        body = f"//Cannot capture deleted function {func.path}"
        # TODO handle template funcs

    return (preDecl, body)

@MemRttiRenderer(cx_ast.StaticFuncInfo)
def render_memStaticFunc(func:cx_ast.StaticFuncInfo):
    # Don't write template callables
    if func.path.isDynamicallyInstanced: return None

    # Detect how to reference
    #if func.visibility != cx_ast.Member.Visibility.Public:
    pubCastKey = makePubCastKey(func)
    formatter = {
        "key": pubCastKey,
        "TClass": func.path.parent,
        "returnType": func.returnTypeName,
        "params": ", ".join([str(i.typeName) for i in func.parameters]),
        "name": func.path.ownName.base
    }
    preDecl = "\n".join([
        'PUBLIC_CAST_DECLARE_KEY_BARE({key});'.format_map(formatter),
		'template<> struct ::public_cast::_type_lut<PUBLIC_CAST_KEY_OF({key})>'.format_map(formatter) + ' { ' + 'using ptr_t = {returnType} (*)({params});'.format_map(formatter) + ' };',
		'PUBLIC_CAST_GIVE_ACCESS_BARE({key}, {TClass}, {name});'.format_map(formatter)
    ])
    pubReference = f"DO_PUBLIC_CAST({pubCastKey})"
    #else:
    #    preDecl = f"//{func.path} is already public: no need for public_cast"
    #    pubReference = func.path
   
    #paramNames = [i.displayName for i in func.parameters] # TODO implement name capture on C++ side
    body = f"builder.addStaticFunction(stix::StaticFunction::make({pubReference}), \"{func.path.ownName.base}\", {func.visibility});"
    # TODO handle template funcs

    return (preDecl, body)


########################## Annotation misc ##########################

def evalAsBool(val:str):
    if val.lower() in ["y", "yes", "true" , "enable" , "enabled" , "on" ]: return True
    if val.lower() in ["n", "no" , "false", "disable", "disabled", "off"]: return False
    assert False

def searchAnnotations(node:cx_ast.ASTNode, selector, includeParents=True) -> cx_ast.Annotation|None:
    for i in node.children:
        if isinstance(i, cx_ast.Annotation) and selector(i): return i

    if includeParents and node.owner != None: return searchAnnotations(node.owner, selector, includeParents=True)
    return None

########################## Annotation: CDO enable/backend ##########################

def detectImageCaptureStatus(ty:cx_ast.StructInfo):
    prefix = "stix::do_image_capture"
    annot = searchAnnotations(ty, lambda a: a.text.startswith(prefix))
    if annot != None:
        status = annot.text[len(prefix)+1:]
        if status == "default": status = RttiGenerator.defaultImageCaptureStatus
    else:
        status = RttiGenerator.defaultImageCaptureStatus
    return evalAsBool(status)

def detectImageCaptureBackend(ty:cx_ast.StructInfo):
    prefix = "stix::image_capture_backend"
    annot = searchAnnotations(ty, lambda a: a.text.startswith(prefix))
    if annot != None:
        backend = annot.text[len(prefix)+1:]
        if backend == "default": backend = RttiGenerator.defaultImageCaptureBackend
    else:
        backend = RttiGenerator.defaultImageCaptureBackend
    return backend

imageCaptureBackend_t = typing.Callable[[cx_ast.StructInfo], str]
imageCaptureBackends:dict[str, imageCaptureBackend_t] = dict()
def ImageCaptureBackend(name:str):
    """
    Helper to auto-register renderer functions
    """
    def registrar(func:imageCaptureBackend_t):
        assert name not in imageCaptureBackends.keys()
        imageCaptureBackends[name] = func
        return func
    return registrar

@ImageCaptureBackend("cdo")
def __renderImageCapture_cdo(ty:cx_ast.StructInfo):
    # Gather valid constructors
    ctors = [i for i in ty.children if isinstance(i, cx_ast.ConstructorInfo) and not i.deleted]
    isGeneratorFnFriended = ty.isFriended(lambda i: i.friendedSymbolPath.startsWith(cx_ast.SymbolPath()+"thunk_utils"))
    if not isGeneratorFnFriended: ctors = [i for i in ctors if i.visibility == cx_ast.Member.Visibility.Public] # Filter to what we can see
    ctors.sort(key=lambda i: len(i.parameters))
        
    defaultCtor = next((i for i in ctors if len(i.parameters)==0), None)
    if defaultCtor == None:
        return f"#error {ty.path} has no accessible CDO-compatible constructor, and cannot have its image snapshotted"

    return f"builder.captureClassImage_v1<{ty.path}>();"

@ImageCaptureBackend("disassembly")
def __renderImageCapture_disassembly(ty:cx_ast.StructInfo):
    # Gather valid constructors
    ctors = [i for i in ty.children if isinstance(i, cx_ast.ConstructorInfo) and not i.deleted]
    isGeneratorFnFriended = ty.isFriended(lambda i: i.friendedSymbolPath.startsWith(cx_ast.SymbolPath()+"thunk_utils"))
    if not isGeneratorFnFriended: ctors = [i for i in ctors if i.visibility == cx_ast.Member.Visibility.Public] # Filter to what we can see
    ctors.sort(key=lambda i: len(i.parameters))

    if len(ctors) == 0:
        return f"#error {ty.path} has no accessible Disassembly-compatible constructor, and cannot have its image snapshotted"
        
    ctorParamArgs = [str(ty.path)] + [str(i.typeName) for i in ctors[0].parameters]
    return f"builder.captureClassImage_v2<{ ', '.join(ctorParamArgs) }>();"



if __name__ == "__main__":
    import sys

    timings.switchTask(timings.TASK_ID_INIT)
    parser = argparse.ArgumentParser(
        prog=os.path.basename(__file__),
        description="STIX generation tool: Embedding RTTI in a C++ binary as a build step"
    )
    RttiGenerator.argparser_add_defaults(parser)
    args = parser.parse_args(sys.argv[1:])

    rttigen = RttiGenerator(args)
    rttigen.configure()
    rttigen.execute()
    