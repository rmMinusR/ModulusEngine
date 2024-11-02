import abc
from enum import Enum
from functools import cached_property
from source_discovery import SourceFile
import config
import typing

class SourceLocation:
    def __init__(this, file: SourceFile, line:int, column:int):
        this.file = file
        this.line = line
        this.column = column
        
    def __repr__(this):
        return f"{this.file.name} : L{this.line}.C{this.column}"


class SymbolPath:
    BasicSegment = str
    class TemplatedSegment:
        def __init__(this, base:str):
            this.base = base
            this.templateParams:list[ str|SymbolPath ] = [] # SymbolPath for type arguments, str for values (int, char, enum, etc)
            
        def __str__(this): return this.base+"<" + ", ".join(str(i) for i in this.templateParams) + ">"
        def __repr__(this): return this.__str__()

        def __eq__(this, rhs):
            if not isinstance(rhs, SymbolPath.TemplatedSegment): return False
            if this.base != rhs.base or len(this.templateParams) != len(rhs.templateParams): return False
            return all(this.templateParams[i]==rhs.templateParams[i] for i in range(len(this.templateParams)))
    class Anonymous:
        def __init__(this, location:SourceLocation, _type:str = "anonymous"):
            this.location = location
            this._type = _type
            
        def __str__(this): return f"({this._type} at {this.location})"
        def __repr__(this): return this.__str__()
        
        def __eq__(this, rhs):
            return isinstance(rhs, SymbolPath.Anonymous) and this.location == rhs.location
    class CallParameterized: # Used to disambiguate method groups with argument or constness overloading
        def __init__(this, base:"SymbolPath.SegmentAny", args:"list[SymbolPath]", affixes:list[str]):
            this.base = base
            this.args = args
            this.affixes = affixes # const/volatile. NOT virtual/override.
            this.affixes.sort()
            
        def __str__(this):
            out = this.base+"(" + ", ".join([str(i) for i in this.args]) + ")"
            if len(this.affixes) > 0: out += " "+" ".join(this.affixes)
            return out
        def __repr__(this): return this.__str__()

        def __eq__(this, rhs):
            if not isinstance(rhs, SymbolPath.CallParameterized): return False
            if this.base != rhs.base or len(this.args) != len(rhs.args) or len(this.affixes) != len(rhs.affixes): return False
            return all(this.affixes[i]==rhs.affixes[i] for i in range(len(this.affixes))) \
               and all(this.args[i]==rhs.args[i] for i in range(len(this.args)))

    SegmentAny = BasicSegment|TemplatedSegment|Anonymous|CallParameterized

    def __init__(this):
        this.__parts:list[SymbolPath.SegmentAny] = []

    def __str__(this): return "::" + "::".join([str(i) for i in this.__parts])
    def __repr__(this): return this.__str__()
    
    def __hash__(this): return hash(this.__str__())

    def __div__(this, rhs:"SymbolPath.SegmentAny"):
        out = SymbolPath()
        out.__parts.extend(this.__parts)
        out.__parts.append(rhs)
        return out
    def __add__(this, rhs): return this.__div__(rhs)
    
    def __eq__(this, rhs):
        if not isinstance(rhs, SymbolPath): return False
        return this.__parts == rhs.__parts
        #if len(this.__parts) != len(rhs.__parts): return False
        #return all(this.__parts[i]==rhs.__parts[i] for i in range(len(this.__parts)))
    
    def __len__(this):
        return len(this.__parts)

    def startsWith(this, other:"SymbolPath"):
        if len(other.__parts) > len(this.__parts): return False
        return other.__parts == this.__parts[:len(other.__parts)]

    @cached_property
    def parent(this) -> "SymbolPath|None":
        if len(this.__parts) > 1:
            out = SymbolPath()
            out.__parts = this.__parts[:-1]
            return out
        else: return None
        
    @cached_property
    def ownName(this): return this.__parts[-1]

    @cached_property
    def name(this): return str(this.__parts[-1])
    
    @cached_property # NOTE: Doesn't do visibility checks
    def referenceable(this):
        return not any(isinstance(i, SymbolPath.Anonymous) for i in this.__parts)

    @cached_property
    def isDynamicallyInstanced(this): # Anything within a template doesn't have a defined location, and is instead compiler generated upon instantiation
        return any(isinstance(i, SymbolPath.TemplatedSegment) for i in this.__parts)

    #@staticmethod
    #def naive_parse(raw:str): # TODO gets completely screwed if someone does a boolean less-than compare. Scrap?
    #    out = SymbolPath()
    #    braces = ("()", "<>", "{}", "[]", '""', "''") # Hope nobody uses trigraphs...
    #    
    #    if raw.startswith("::"): raw = raw[2:]
    #
    #    while len(raw) > 0:
    #        braceStack = []
    #        for idx in range(1, len(raw)):
    #            if len(braceStack) > 0 and any( braceStack[-1]==brace[0] and raw[idx] == brace[1] for brace in braces ):
    #                # Closing brace or quote
    #                braceStack.pop()
    #            elif any( raw[idx] == brace[0] for brace in braces ):
    #                # Opening brace or quote
    #                braceStack.append(raw[idx])
    #            elif len(braceStack)==0 and idx<len(raw)-1 and raw[idx:idx+2] == "::":
    #                break
    #        out.__parts.append(raw[:idx])
    #        raw = raw[idx+2:]
    #            
    #    return out



class ASTNode:
    __metaclass__ = abc.ABCMeta

    def __init__(this, path:SymbolPath|None, location: SourceLocation, isDefinition:bool):
        this.path = path
        this.transient:bool = False # Used by implicits added during the link phase

        this.declarationLocations:list[SourceLocation] = []
        this.definitionLocation:SourceLocation = None
        if isDefinition: this.definitionLocation = location
        else: this.declarationLocations.append(location)
        
        this.owner:ASTNode|None = None
        this.children = []

    def __str__(this):
        typeStr = str(type(this))
        typeStr = typeStr[len("<class '"):-len("'>")].split(".")[-1]
        return f"{this.path} ({typeStr})"
    
    def __repr__(this):
        return str(this)
    
    def merge(this, new:"ASTNode"): # Called on existing instance
        # Already aware of this symbol: check that previous record was a declaration
        if new.definitionLocation != None:
            assert this.definitionLocation == None, f"Node {this} defined multiple times!"

        # Merge locational data
        this.definitionLocation = new.definitionLocation
        this.declarationLocations.extend(new.declarationLocations)
        
        # Move over children
        this.children.extend(new.children)
        for i in new.children: i.owner = this

    @staticmethod
    def allowMultiple():
        return False

    def link(this, module: "Module"):
        pass
            
    def latelink(this, module: "Module"):
        pass


class Module:
    def __init__(this):
        this.contents:dict[SymbolPath, ASTNode|list[ASTNode]] = dict()
        this.byType:dict[type, list[ASTNode]] = dict()
        this.__linked = False
        this.__linking = False
        this.__concurrentlyAdded = []

        # Transient, only used during linking
        # Dependency injection by reader implementation
        this.externals:"dict[SymbolPath, Any]" = {}
        this.findExternal:Callable[[SymbolPath], list[ASTNode]] = None
        
    def __getstate__(this):
        out = [i for i in this.contents.values() if isinstance(i, ASTNode) and not i.transient] \
            + [[i for i in grp if not i.transient] for grp in this.contents.values() if isinstance(grp, list)]
        out.sort(key=lambda node: str(node.path if isinstance(node, ASTNode) else node[0].path))
        return out
    
    def __setstate__(this, vals):
        this.contents = dict()
        this.byType = dict()
        this.__concurrentlyAdded = []

        def _put_byType(i:ASTNode):
            if type(i) not in this.byType.keys(): this.byType[type(i)] = []
            this.byType[type(i)].append(i)

        for i in vals:
            if isinstance(i, ASTNode):
                this.contents[i.path] = i
                _put_byType(i)
            else:
                if not i[0].path in this.contents.keys(): this.contents[i[0].path] = []
                this.contents[i[0].path].extend(i)
                for j in i: _put_byType(j)

    def register(this, node:ASTNode):
        if this.__linking:
            this.__concurrentlyAdded.append(node) # Defer
        else:
            assert not this.__linked, "Cannot register new nodes after linking"
            this.__registerInternal(node)
    
    def __registerInternal(this, node:ASTNode):
        # Place in fast-lookup contents tree
        if not node.allowMultiple():
            # Aware of symbol: merge into existing entry
            if node.path in this.contents.keys(): this.contents[node.path].merge(node)
            
            # Not aware of symbol: nothing to do
            else: this.contents[node.path] = node
        else:
            # Duplicate-allowed symbols are complete and utter chaos
            if not node.path in this.contents.keys(): this.contents[node.path] = []
            this.contents[node.path].append(node)
        
        # Place in by-type lookup
        if not type(node) in this.byType.keys(): this.byType[type(node)] = []
        this.byType[type(node)].append(node)

    def remove(this, node:ASTNode):
        del this.contents[node.path]
        this.byType[type(node)].remove(node)
        if node.owner != None: node.owner.children.remove(node)

    def linkAll(this):
        this.__linked = True
        this.__linking = True
        
        def _reducing_invoke(obj, op):
            if isinstance(obj, list):
                for i in obj: op(i)
            else: op(obj)
        
        # Nasty hack to prevent symbol duplication
        for v in this.contents.values(): _reducing_invoke(v, lambda o: setattr(o, "children", []))
        def _associate_owner_with_child(o):
            if o.owner != None: o.owner.children.append(o)
        for v in this.contents.values(): _reducing_invoke(v, _associate_owner_with_child)
        
        # Link explicit symbols
        for v in this.contents.values():
            try: _reducing_invoke(v, lambda o:o.link(this))
            except:
                config.logger.critical(f"While linking {v[0].path if isinstance(v, list) else v.path}")
                raise
        
        # Link implicit symbols added in link()
        while len(this.__concurrentlyAdded) > 0:
            node = this.__concurrentlyAdded.pop(0)
            this.__registerInternal(node)
            
            # Handle parent/children connection
            parent = this.find(node.path.parent)
            if parent != None:
                parent.children.append(node)
                node.owner = parent
                
            # Link
            try: _reducing_invoke(node, lambda o:o.link(this))
            except:
                config.logger.critical(f"While linking implicit symbol {node[0].path if isinstance(node, list) else node.path} generated during link step")
                raise    
            
        # Late-link explicit symbols
        for v in this.contents.values():
            try: _reducing_invoke(v, lambda o:o.latelink(this))
            except:
                config.logger.critical(f"While late-linking {v[0].path if isinstance(v, list) else v.path}")
                raise
        
        # Fully link implicit symbols added in latelink()
        while len(this.__concurrentlyAdded) > 0:
            node = this.__concurrentlyAdded.pop(0)
            this.__registerInternal(node)
            
            # Handle parent/children connection
            parent = this.find(node.path.parent)
            if parent != None:
                parent.children.append(node)
                node.owner = parent
                
            # Link and late-link
                
            try: _reducing_invoke(node, lambda o:o.link(this))
            except:
                config.logger.critical(f"While linking {node[0].path if isinstance(node, list) else node.path} generated during late-link step")
                raise
            
            try: _reducing_invoke(node, lambda o:o.latelink(this))
            except:
                config.logger.critical(f"While late-linking {node[0].path if isinstance(node, list) else node.path} generated during late-link step")
                raise
        
        this.__linking = False
        
    def find(this, path:SymbolPath) -> ASTNode|list[ASTNode]|None:
        assert this.__linked, "Can only be called after or during linking"
        if path in this.contents.keys():
            # We own this symbol and got it on our main pass
            return this.contents[path]
        else:
            # Try externals
            externals = this.findExternal(path)
            if len(externals) != 0:
                return externals if len(externals)>1 else externals[0]
            else:
                # Nope, not an external either
                return None
                


class Namespace(ASTNode):
    def __init__(this, path:SymbolPath, location: SourceLocation):
        ASTNode.__init__(this, path, location, False)
        
    #@staticmethod
    #def allowMultiple():
    #    return True


class Annotation(ASTNode):
    def __init__(this, ownerPath:SymbolPath, text:str, location:SourceLocation):
        ASTNode.__init__(this, ownerPath+SymbolPath.Anonymous(location, _type=f"annotation '{text}'"), location, True)
        this.text = text
        
    @staticmethod
    def allowMultiple():
        return True
    

class TypeInfo(ASTNode):
    def __init__(this, path:SymbolPath, location: SourceLocation, isDefinition:bool, isAbstract:bool):
        ASTNode.__init__(this, path, location, isDefinition)
        this.isAbstract = isAbstract
        
    @property
    def immediateParents(this):
        return (i for i in this.children if isinstance(i, ParentInfo))

    def link(this, module:"Module"):
        if this.definitionLocation != None: # Somehow this is being called multiple times per type??? This causes multiple ctors to be registered in a single run but somehow they sneak in after verification...
            # Implicit default ctor
            if not any((isinstance(i, ConstructorInfo) for i in this.children)):
                ctorPathPart = SymbolPath.CallParameterized(str(this.path.ownName), [], [])
                implicitDefaultCtor = ConstructorInfo(this.path+ctorPathPart, this.definitionLocation, True, False, True, Member.Visibility.Public)
                implicitDefaultCtor.transient = True
                module.register(implicitDefaultCtor)
                this.children.append(implicitDefaultCtor)

            # Implicit default ctor
            if not any((isinstance(i, DestructorInfo) for i in this.children)):
                dtorPathPart = SymbolPath.CallParameterized(f"~{this.path.ownName}", [], [])
                implicitDefaultDtor = DestructorInfo(this.path+dtorPathPart, this.definitionLocation, True, Member.Visibility.Public, False, False, False, True)
                implicitDefaultDtor.transient = True
                module.register(implicitDefaultDtor)
                this.children.append(implicitDefaultDtor)
        
        super().link(module)

    def find(this, memberName, searchParents=False) -> "Member":
        for i in this.children:
            if isinstance(i, Member) and i.path.ownName == memberName:
                return i
            
        if searchParents:
            return this.findInParents(memberName)
            
        return None
    
    def findInParents(this, memberName) -> "Member":
        for i in this.children:
            if isinstance(i, ParentInfo):
                potentialMatch = i.parentType.find(memberName, searchParents=True)
                if potentialMatch != None: return potentialMatch
        return None

    def isFriended(this, selector:typing.Callable[["FriendInfo"], bool]):
        return any((
            isinstance(i, FriendInfo) and selector(i)
            for i in this.children
        ))
        
     
        
class Member(ASTNode):
    
    class Visibility(str, Enum):
        Public    = "MemberVisibility::Public"
        Protected = "MemberVisibility::Protected"
        Private   = "MemberVisibility::Private"

    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, visibility:Visibility):
        ASTNode.__init__(this, path, location, isDefinition)
        this.visibility = visibility
        this.owner = None
    

class MaybeVirtual(Member):
    __metaclass__ = abc.ABCMeta
    
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, visibility:Member.Visibility, isExplicitVirtual:bool, isExplicitOverride:bool):
        Member.__init__(this, path, location, isDefinition, visibility)
        this.isExplicitVirtual = isExplicitVirtual
        this.isExplicitOverride = isExplicitOverride
        this.inheritedFrom = None
        this.inheritedVersion = None
        this.isVirtual = None
        this.isOverride = None
        
    def latelink(this, module: Module):
        this.inheritedVersion = module.find(this.path.parent).findInParents(this.path.ownName)
        
        def __isVirtual (v:MaybeVirtual): return v.isExplicitVirtual  or (__isVirtual (v.inheritedVersion) if v.inheritedVersion != None else False)
        def __isOverride(v:MaybeVirtual): return v.isExplicitOverride or (__isOverride(v.inheritedVersion) if v.inheritedVersion != None else False)
        this.isVirtual = __isVirtual(this)
        this.isOverride = __isOverride(this)


class Callable(ASTNode):
    class Parameter(ASTNode):
        def __init__(this, path:SymbolPath, location:SourceLocation, index:int, typeName:"QualifiedType"):
            ASTNode.__init__(this, path.parent+SymbolPath.Anonymous(location, _type=f"parameter #{index}: {typeName} {path.ownName}"), location, False)
            this.index = index
            this.typeName = typeName
            this.type:TypeInfo|None = None

        def latelink(this, module:Module):
            this.type = module.find(this.typeName)
            
        def merge(this, new:"Callable.Parameter"):
            assert this.type == new.type
            assert this.index == new.index

            # If named in new copy but not current, update name
            if isinstance(this.path.ownName, SymbolPath.Anonymous) and not isinstance(new.path.ownName, SymbolPath.Anonymous):
                this.path = new.path

            super().merge(new)

        @staticmethod
        def allowMultiple():
            return True
            
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, returnTypeName:str, deleted:bool, inline:bool):
        ASTNode.__init__(this, path, location, isDefinition)
        this.returnTypeName = returnTypeName
        this.deleted = deleted
        this.inline = inline
        if inline and isDefinition: this.declarationLocations.append(location)

    def link(this, module:Module):
         super().link(module)
         assert isinstance(this.path.ownName, SymbolPath.CallParameterized)

    def merge(this, new:"Callable"):
        # We need to de-duplicate parameters caused by multiple decls/defs of the function

        # Sanity check: param types match
        assert [i.type for i in this.parameters] == [i.type for i in new.parameters], f"Parameter mismatch: {this} vs {new}"
        
        # Merge params
        for i in range(len(this.parameters)): this.parameters[i].merge(new.parameters[i])

        # Prevent param duplication and proceed with standard merge
        new.children = [i for i in new.children if not isinstance(i, Callable.Parameter)]
        super().merge(new)

    @property
    def parameters(this) -> list[Parameter]:
        return [i for i in this.children if isinstance(i, Callable.Parameter)]
   

class StaticFuncInfo(Callable, Member):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, visibility:Member.Visibility, returnTypeName:str, deleted:bool, inline:bool):
        Callable.__init__(this, path, location, isDefinition, returnTypeName, deleted, inline)
        Member.__init__(this, path, location, isDefinition, visibility)
        

class GlobalFuncInfo(Callable):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, returnTypeName:str, deleted:bool, inline:bool):
        Callable.__init__(this, path, location, isDefinition, returnTypeName, deleted, inline)
    

class MemFuncInfo(MaybeVirtual, Callable):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool,
                visibility:Member.Visibility, isExplicitVirtual:bool, isExplicitOverride:bool,\
                returnTypeName:str, deleted:bool, inline:bool,
                isThisObjConst:bool, isThisObjVolatile:bool):
        MaybeVirtual.__init__(this, path, location, isDefinition, visibility, isExplicitVirtual, isExplicitOverride)
        Callable    .__init__(this, path, location, isDefinition, returnTypeName, deleted, inline)
        this.isThisObjConst = isThisObjConst
        this.isThisObjVolatile = isThisObjVolatile
        

class ConstructorInfo(Member, Callable):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, deleted:bool, inline:bool, visibility:Member.Visibility):
        Member  .__init__(this, path, location, isDefinition, visibility)
        Callable.__init__(this, path, location, isDefinition, None, deleted, inline)


class DestructorInfo(MaybeVirtual, Callable):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool,
                visibility:Member.Visibility, isExplicitVirtual:bool, isExplicitOverride:bool,\
                deleted:bool, inline:bool):
        MaybeVirtual.__init__(this, path, location, isDefinition, visibility, isExplicitVirtual, isExplicitOverride)
        Callable    .__init__(this, path, location, isDefinition, None, deleted, inline)

    def latelink(this, module: Module):
        def __getAllDtors(ty:TypeInfo) -> list[DestructorInfo]:
            out = []
            for p in ty.children:
                if isinstance(p, ParentInfo):
                    dtor = next((i for i in p.parentType.children if isinstance(i, DestructorInfo)), None)
                    if dtor != None: out.append(dtor)
                    out += __getAllDtors(p.parentType)
            return out
        
        this.isVirtual = this.isExplicitVirtual or any(i.isExplicitVirtual for i in __getAllDtors(this.owner))
        this.isOverride = any(i.isExplicitVirtual for i in __getAllDtors(this.owner)[1:]) # Skip virtual check on self, only check if parents are virtual


class GlobalVarInfo(ASTNode):
    def __init__(this, path:SymbolPath, location: SourceLocation, isDefinition:bool, typeName:str|None):
        ASTNode.__init__(this, path, location, isDefinition)
        this.typeName = typeName
        this.type = None
        
    def link(this, module:Module):
        super().link(module)
        this.type = module.find(this.typeName)


class StaticVarInfo(Member):
    def __init__(this, path:SymbolPath, location:SourceLocation, isDefinition:bool, visibility:Member.Visibility, typeName:str|None):
        Member.__init__(this, path, location, isDefinition, visibility)
        this.typeName = typeName
        this.type = None
        
    def link(this, module:Module):
        super().link(module)
        this.type = module.find(this.typeName)


class FieldInfo(Member):
    def __init__(this, path:SymbolPath, location:SourceLocation, visibility:Member.Visibility, typeName:str|None):
        Member.__init__(this, path, location, True, visibility)
        this.typeName = typeName
        this.type = None
        
    def link(this, module:Module):
        super().link(module)
        this.type = module.find(this.typeName)
        

class ParentInfo(Member):
    class Virtualness(str, Enum):
        NonVirtual       = "ParentInfo::Virtualness::NonVirtual"
        VirtualExplicit  = "ParentInfo::Virtualness::VirtualExplicit"
        VirtualInherited = "ParentInfo::Virtualness::VirtualInherited"

    def __init__(this, ownerPath:SymbolPath, parentTypePath:SymbolPath, location:SourceLocation, visibility:Member.Visibility, explicitlyVirtual:bool):
        Member.__init__(this, ownerPath+SymbolPath.Anonymous(location, _type=f"parent {parentTypePath}"), location, True, visibility)
        this.parentTypePath = parentTypePath
        this.parentType = None
        this.explicitlyVirtual = explicitlyVirtual
        #this.isVirtuallyInherited = None
    
    def link(this, module: Module):
        super().link(module)
        this.parentType = module.find(this.parentTypePath)
        if this.parentType == None:
            config.logger.warn(f"Could not find {this.parentTypePath}, parent of {this.owner.path}")

    def latelink(this, module: Module):
        #if this.explicitlyVirtual:
        #    this.isVirtuallyInherited = True
        #else:
        #    def __immediateParents(ty:TypeInfo): return (i for i in ty if isinstance(i, ParentInfo))
        #    def __isVirtuallyInherited(p:ParentInfo):
        #        
        #    def __isVirtual(ty:TypeInfo):
        #        return any(( i.explicitlyVirtual for i in __immediateParents(ty) )) or \
        #               any(( __isVirtual(i.parentType) for i in __immediateParents(ty) ))
        #    this.isVirtuallyInherited = __isVirtual(this)
        pass


class FriendInfo(Member):
    def __init__(this, ownerPath:SymbolPath, friendedSymbolPath:SymbolPath, location:SourceLocation, visibility:Member.Visibility):
        Member.__init__(this, ownerPath+SymbolPath.Anonymous(location, _type=f"friend {friendedSymbolPath}"), location, True, visibility)
        this.friendedSymbolPath = friendedSymbolPath
        this.friendedSymbol = None

    def latelink(this, module: Module):
        this.friendedSymbol = module.find(this.friendedSymbolPath)


class TemplateParameter(ASTNode):
    def __init__(this, path:SymbolPath, location:SourceLocation, index:int, paramType:str, defaultValue:str|None): # paramType is one of: typename, concept, class, struct, int... (or any other templatable)
        this.paramName = path.ownName
        ASTNode.__init__(this, path.parent+SymbolPath.Anonymous(location, _type=f"template parameter {paramType} {path.ownName}"), location, True)
        this.index = index
        this.paramType = paramType
        this.defaultValue = defaultValue



class QualifiedType:
    class PointerSpec(str, Enum):
        NONE = "{base}{template_params} {qualifiers} {name}"
        POINTER = "{base}* {qualifiers} {name}"
        FUNC_POINTER = "{return_type} ({name}* {qualifiers})({func_args})"
        REFERENCE = "{base}{template_params}& {name} {qualifiers}"
        MOVE_REFERENCE = "{base}{template_params}&& {name} {qualifiers}"
        MEM_FIELD_POINTER = "{base}::{name}* {qualifiers}"
        MEM_FUNC_POINTER = "{return_type} ({base}::{name}* {qualifiers})({func_args}) {this_obj_qualifiers}"

    def __init__(this, base:"SymbolPath|QualifiedType|str", qualifiers:list[str]=[], pointer_spec:PointerSpec=PointerSpec.NONE,
                       func_args:"list[QualifiedType]"=[], # All callables
                       return_type:"QualifiedType"=None, # Functions
                       this_obj_qualifiers:list[str]=[], # Nonstatic member functions only
                       template_params:"list[QualifiedType]"=[]): # Only valid if nonpointer
        this.base = base
        this.qualifiers = qualifiers
        this.pointer_spec = pointer_spec
        this.return_type = return_type
        this.this_obj_qualifiers = this_obj_qualifiers
        this.func_args = func_args
        this.template_params = template_params
        
        this.qualifiers.sort()
        
        if isinstance(base, QualifiedType):
            assert base.pointer_spec not in [QualifiedType.PointerSpec.REFERENCE, QualifiedType.PointerSpec.MOVE_REFERENCE], "Pointer-to-reference is not valid syntax"
        
    def __get_formatter(this):
        return {
            "base": this.base,
            "qualifiers": "".join(" "+str(i) for i in this.qualifiers),
            "return_type": this.return_type if this.return_type != None else "",
            "func_args": ", ".join(str(i) for i in this.func_args),
            "this_obj_qualifiers": "".join(" "+str(i) for i in this.this_obj_qualifiers),
            "name": "",
            "template_params": "" if len(this.template_params)==0 else ("<" + ", ".join(str(i) for i in this.template_params) + ">"),
        }

    def __str__(this):
        return this.pointer_spec.format(**this.__get_formatter()).strip().replace("  ", " ")
    
    def __repr__(this): return this.__str__()
    
    def __eq__(this, other):
        if not isinstance(other, QualifiedType): return False
        
        # If any form of func ptr, check args and return type
        if this.pointer_spec in [QualifiedType.PointerSpec.FUNC_POINTER, QualifiedType.PointerSpec.MEM_FUNC_POINTER]:
            if this.return_type != other.return_type or this.func_args != other.func_args: return False

        # Check basics
        return this.base == other.base \
           and this.pointer_spec == other.pointer_spec \
           and set(this.qualifiers) == set(other.qualifiers)
    
    def __hash__(this):
        if hasattr(this, "__hash_val"): return getattr(this, "__hash_val")
        
        val = hash(this.base) ^ hash(this.pointer_spec)
        for i in this.qualifiers: val ^= hash(i)
        if this.pointer_spec in [QualifiedType.PointerSpec.FUNC_POINTER, QualifiedType.PointerSpec.MEM_FUNC_POINTER]:
            val ^= hash(this.return_type)
            val ^= hash(this.func_args)
        
        setattr(this, "__hash_val", val)
        return val

    def instantiate_var(this, name) -> str:
        formatter = this.__get_formatter()
        formatter["name"] = name
        return this.pointer_spec.format(**this.__get_formatter())
