﻿from typing import Generator, Iterator
from clang.cindex import *
from textwrap import indent
import abc
import re
import zlib, hashlib


class log:
    trace = lambda x: print("[TRACE] "+x)
    info  = lambda x: print("[ INFO] "+x)
    warn  = lambda x: print("[ WARN] "+x)
    error = lambda x: print("[ERROR] "+x)


def _getAbsName(target: Cursor) -> str:
	if type(target.semantic_parent) == type(None) or target.semantic_parent.kind == CursorKind.TRANSLATION_UNIT:
		# Root case: Drop translation unit name
		return target.displayname
	else:
		# Loop case
		return _getAbsName(target.semantic_parent) + "::" + target.displayname


class Symbol:
    __metaclass__ = abc.ABCMeta

    def __init__(this, cursor: Cursor):
        this.__astRepr = cursor

    @property
    def astKind(this):
        return this.__astRepr.kind
        
    @property
    def absName(this) -> str:
        if not hasattr(this, "__cachedAbsName"):
            this.__cachedAbsName = _getAbsName(this.__astRepr)
        return this.__cachedAbsName

    @property
    def relName(this) -> str:
        return this.__astRepr.displayname

    @property
    def renderedName(this) -> str:
        if not hasattr(this, "__cachedRenderedName"):
            #this.__cachedRenderedName = "__generatedRTTI_"+re.sub("[^a-zA-Z0-9_]+", "_", this.absName)
            #this.__cachedRenderedName = "_"+hex(zlib.crc32(this.absName.encode("utf-8")))[2::]+"_generatedRTTI_"+re.sub("[^a-zA-Z0-9_]+", "_", this.absName) # Fixes variable length limit
            this.__cachedRenderedName = "__generatedRTTI_"+hashlib.sha256(this.absName.encode("utf-8")).hexdigest()[:8]
        return this.__cachedRenderedName

    @abc.abstractmethod
    def forwardRender(this):
        return


class Ownable:
    __metaclass__ = abc.ABCMeta

    def __init__(this, module, cursor: Cursor):
        this.__owner = module.lookup(cursor.semantic_parent) if TypeInfo.matches(cursor.semantic_parent) else None

    @property
    def isGlobal(this):
        return this.__owner == None

    @property
    def isMember(this):
        return this.__owner != None

    @property
    def owner(this):
        assert this.isMember, f"{this.absName} has no owner"
        return this.__owner


class FuncInfo(Symbol, Ownable):
    def __init__(this, module, cursor: Cursor):
        Symbol.__init__(this, cursor)
        Ownable.__init__(this, module, cursor)
        assert FuncInfo.matches(cursor), f"{cursor.kind} {this.absName} is not a function"

        # TODO deduce address

    @staticmethod
    def matches(cursor: Cursor):
        return cursor.kind in [
            CursorKind.CXX_METHOD,
            CursorKind.FUNCTION_DECL,
            CursorKind.FUNCTION_TEMPLATE
        ]

    def forwardRender(this):
        return "FuncInfo "+this.renderedName+" = FuncInfo(); // "+this.absName # TODO implement


class VarInfo(Symbol, Ownable):
    def __init__(this, module, cursor: Cursor):
        Symbol.__init__(this, cursor)
        Ownable.__init__(this, module, cursor)
        assert VarInfo.matches(cursor), f"{cursor.kind} {this.absName} is not a variable"

        # TODO deduce address (global or relative)

    @staticmethod
    def matches(cursor: Cursor):
        return cursor.kind in [
            CursorKind.FIELD_DECL,
            CursorKind.VAR_DECL
        ]

    def forwardRender(this):
        return "VarInfo "+this.renderedName+" = VarInfo(); // "+this.absName # TODO implement


class TypeInfo(Symbol):
    def __init__(this, module, cursor: Cursor):
        super().__init__(cursor)
        assert TypeInfo.matches(cursor), f"{cursor.kind} {cursor.displayname} is not a type"

        this.__contents = list()
    
    def register(this, obj):
        assert not any([obj.absName == i.absName for i in this.__contents]), f"Tried to register {obj.absName} ({obj.astKind}), but it was already registered!"
        log.trace(f"Registering member symbol {obj.absName} of type {obj.astKind}")
        this.__contents.append(obj)
    
    @staticmethod
    def matches(cursor: Cursor):
        return cursor.kind in [
            CursorKind.CLASS_DECL, CursorKind.CLASS_TEMPLATE, CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION,
			CursorKind.STRUCT_DECL, CursorKind.UNION_DECL
        ]

    def forwardRender(this):
        memberFwdDecls = "\n".join([i.forwardRender() for i in this.__contents])

        fmtRef = lambda member, isLast: "&"+member.renderedName+("," if not isLast else " ")+" // "+member.relName
        memberRefDecls = "\n".join([fmtRef(i, False) for i in this.__contents[:-2]])
        if len(this.__contents) != 0:
            memberRefDecls += "\n"+fmtRef(this.__contents[-1], True)

        ownDecl = f"""TypeInfo {this.renderedName} = TypeInfo("{this.relName}", "{this.absName}", typeid({this.absName}), sizeof({this.absName}), (vtable_ptr)nullptr, """+"{\n"
        ownDecl += indent(memberRefDecls, ' '*4)+"\n"
        ownDecl += "});"

        return memberFwdDecls + "\n" + ownDecl


class Module:
    def __init__(this):
        this.__contents = dict()

    def parse(this, cursor: Cursor):
        matchedType = next((i for i in [VarInfo, FuncInfo, TypeInfo] if i.matches(cursor)), None)
        if matchedType != None:
            v = matchedType(this, cursor)

            if not isinstance(v, Ownable) or v.isGlobal:
                this.register(v) # Global or static
            else:
                v.owner.register(v) # Members

            if isinstance(v, TypeInfo):
                for i in cursor.get_children():
                    this.parse(i)
        else:
            log.warn(f"Skipping symbol {_getAbsName(cursor)} of unhandled type {cursor.kind}")

    def register(this, obj):
        assert obj.absName not in this.__contents.keys(), f"Tried to register {obj.absName} ({obj.astKind}), but it was already registered!"
        log.trace(f"Registering global symbol {obj.absName} of type {obj.astKind}")
        this.__contents[obj.absName] = obj

    def lookup(this, key: str | Cursor):
        if isinstance(key, Cursor):
            key = _getAbsName(key)
        return this.__contents[key]

    @property
    def types(this) -> Generator[TypeInfo, None, None]:
        for i in this.__contents:
            if isinstance(i, TypeInfo):
                yield i

    @property
    def globals(this) -> Generator[VarInfo, None, None]: # NOTE: Includes statics inside classes
        for i in this.__contents:
            if isinstance(i, VarInfo):
                yield i

    @property
    def functions(this) -> Generator[FuncInfo, None, None]:
        for i in this.__contents:
            if isinstance(i, FuncInfo):
                yield i

    @property
    def renderedName(this) -> str:
        if not hasattr(this, "__cachedRenderedName"):
            #this.__cachedRenderedName = "__generatedRTTI_"+re.sub("[^a-zA-Z0-9_]+", "_", this.absName)
            #this.__cachedRenderedName = "_"+hex(zlib.crc32(this.absName.encode("utf-8")))[2::]+"_generatedRTTI_"+re.sub("[^a-zA-Z0-9_]+", "_", this.absName) # Fixes variable length limit
            this.__cachedRenderedName = "__generatedRTTI_"+hashlib.sha256(','.join([i.renderedName for i in this.__contents.values()]).encode("utf-8")).hexdigest()[:8]
        return this.__cachedRenderedName

    def render(this):
        out = "\n\n".join([v.forwardRender() for v in this.__contents.values()])+"\n\n"
        out += "Module "+this.renderedName+" = Module({\n"+indent(",\n".join(["&"+i.renderedName for i in this.__contents.values()]), ' '*4)+"\n});"
        out = "namespace {\n"+indent(out, ' '*4)+"\n}" # Wrap in anonymous namespace
        return out
