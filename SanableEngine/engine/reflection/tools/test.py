import typing
import unittest
import os

import source_discovery

class TestDiffs(unittest.TestCase):
    
    @classmethod
    def setUpClass(this):
        this.selfpath = os.path.dirname(os.path.realpath(__file__))

        emptyDir = os.path.join( this.selfpath, "test_data", "diffs", "empty")
        if not os.path.exists(emptyDir): os.mkdir(emptyDir)
            
        os.chdir(emptyDir                                                        ) ; this.data_empty     = source_discovery.Project(".", [])
        os.chdir(os.path.join( this.selfpath, "test_data", "diffs", "simple1"   )) ; this.data_simple1   = source_discovery.Project(".", [])
        os.chdir(os.path.join( this.selfpath, "test_data", "diffs", "simple2"   )) ; this.data_simple2   = source_discovery.Project(".", [])
        os.chdir(os.path.join( this.selfpath, "test_data", "diffs", "includes1" )) ; this.data_includes1 = source_discovery.Project(".", [])
        os.chdir(os.path.join( this.selfpath, "test_data", "diffs", "includes2" )) ; this.data_includes2 = source_discovery.Project(".", [])
        os.chdir(os.path.join( this.selfpath, "test_data", "diffs", "includes3" )) ; this.data_includes3 = source_discovery.Project(".", [])
        os.chdir(this.selfpath)
        assert this.data_simple1.files[0].contents != this.data_simple2.files[0].contents

    def test_fresh(this):
        diff = source_discovery.ProjectDiff(None, this.data_simple2)
        this.assertEqual(len(diff.new     ), 1, "Fresh scan: False negative")
        this.assertEqual(len(diff.removed ), 0, "Fresh scan: False positive")
        this.assertEqual(len(diff.outdated), 0, "Fresh scan: False positive")
        this.assertEqual(len(diff.upToDate), 0, "Fresh scan: False positive")

    def test_added(this):
        diff = source_discovery.ProjectDiff(this.data_empty, this.data_simple2)
        this.assertEqual(len(diff.new     ), 1, "Added file: False negative")
        this.assertEqual(len(diff.removed ), 0, "Added file: False positive")
        this.assertEqual(len(diff.outdated), 0, "Added file: False positive")
        this.assertEqual(len(diff.upToDate), 0, "Added file: False positive")
    
    def test_deleted(this):
        diff = source_discovery.ProjectDiff(this.data_simple1, this.data_empty)
        this.assertEqual(len(diff.new     ), 0, "Deleted file: False positive")
        this.assertEqual(len(diff.removed ), 1, "Deleted file: False negative")
        this.assertEqual(len(diff.outdated), 0, "Deleted file: False positive")
        this.assertEqual(len(diff.upToDate), 0, "Deleted file: False positive")
    
    def test_edited(this):
        diff = source_discovery.ProjectDiff(this.data_simple1, this.data_simple2)
        this.assertEqual(len(diff.new     ), 0, "Edited file: False positive")
        this.assertEqual(len(diff.removed ), 0, "Edited file: False positive")
        this.assertEqual(len(diff.outdated), 1, "Edited file: False negative")
        this.assertEqual(len(diff.upToDate), 0, "Edited file: False positive")
    
    def test_same(this):
        diff = source_discovery.ProjectDiff(this.data_simple1, this.data_simple1)
        this.assertEqual(len(diff.new     ), 0, "No edits: False positive")
        this.assertEqual(len(diff.removed ), 0, "No edits: False positive")
        this.assertEqual(len(diff.outdated), 0, "No edits: False positive")
        this.assertEqual(len(diff.upToDate), 1, "No edits: False negative")

    def test_upstream_same(this):
        diff = source_discovery.ProjectDiff(this.data_includes1, this.data_includes1)
        this.assertEqual(len(diff.new     ), 0, "Upstream edits: False positive")
        this.assertEqual(len(diff.removed ), 0, "Upstream edits: False positive")
        this.assertEqual(len(diff.outdated), 0, "Upstream edits: False positive")
        this.assertEqual(len(diff.upToDate), 2, "Upstream edits: False negative")

    def test_upstream_edited(this):
        diff = source_discovery.ProjectDiff(this.data_includes1, this.data_includes2)
        this.assertEqual(len(diff.new     ), 0, "Upstream edits: False positive")
        this.assertEqual(len(diff.removed ), 0, "Upstream edits: False positive")
        this.assertEqual(len(diff.outdated), 2, "Upstream edits: False negative")
        this.assertEqual(len(diff.upToDate), 0, "Upstream edits: False positive")
        
    def test_upstream_added(this):
        diff = source_discovery.ProjectDiff(this.data_includes3, this.data_includes1)
        this.assertEqual(len(diff.new     ), 0, "Upstream added: False positive")
        this.assertEqual(len(diff.removed ), 0, "Upstream added: False positive")
        this.assertEqual(len(diff.outdated), 1, "Upstream added: False negative")
        this.assertEqual(len(diff.upToDate), 1, "Upstream added: False positive")
    
    def test_upstream_removed(this):
        diff = source_discovery.ProjectDiff(this.data_includes1, this.data_includes3)
        this.assertEqual(len(diff.new     ), 0, "Upstream added: False positive")
        this.assertEqual(len(diff.removed ), 0, "Upstream added: False positive")
        this.assertEqual(len(diff.outdated), 1, "Upstream added: False negative")
        this.assertEqual(len(diff.upToDate), 1, "Upstream added: False positive")


import argparse, cx_ast_tooling, cx_ast, abc
def CallParams(base:str, args:list[cx_ast.SymbolPath], affixes=None) -> cx_ast.SymbolPath.CallParameterized:
    #for idx in range(len(args)):
    #    if isinstance(args[idx], str):
    #        args[idx] = cx_ast.SymbolPath() + args[idx]
            
    return cx_ast.SymbolPath.CallParameterized(base, args, affixes if affixes!=None else [])


ty_int = cx_ast.QualifiedPath("int")
ty_char = cx_ast.QualifiedPath("char")
ty_void = cx_ast.QualifiedPath("void")
ty_const_void = cx_ast.QualifiedPath("void", qualifiers=["const"])
def ty_ptr(ty:cx_ast.QualifiedPath): return cx_ast.QualifiedPath(ty, pointer_spec=cx_ast.QualifiedPath.Spec.POINTER)
def ty_ref(ty:cx_ast.QualifiedPath): return cx_ast.QualifiedPath(ty, pointer_spec=cx_ast.QualifiedPath.Spec.REFERENCE)
ty_const_void_ptr = ty_ptr(ty_const_void)
ty_MyClass = cx_ast.QualifiedPath(cx_ast.SymbolPath()+"MyClass")
ty_MyClass_const = cx_ast.QualifiedPath(cx_ast.SymbolPath()+"MyClass", qualifiers=["const"])
ty_MyClass_ptr = ty_ptr(ty_MyClass)
ty_MyClass_const_ref = ty_ref(ty_MyClass_const)


class TestParser:
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def invoke_parser(target:str, includes:list[str]) -> cx_ast.Module:
        assert False, "Implement this!"
        
    @classmethod    
    def setUpClass(this):
        this.module = this.invoke_parser("test_data/parser", [])
        
    def assertExpectSymbol(this, path_raw:list[cx_ast.SymbolPath.SegmentAny], _ty:type) -> cx_ast.ASTNode|None:
        # Build path
        path = cx_ast.SymbolPath()
        for i in path_raw: path = path+i
        
        sym = this.module.find(path)
        if _ty == None:
            this.assertIsNone(sym, msg=f"Symbol {path} shouldn't exist")
            return None
        elif sym == None:
            this.assertIsNotNone(sym, msg=f"Symbol {path} doesn't exist")
            return None
        else:
            this.assertIsInstance(sym, _ty, msg=f"Symbol {path}: expected {_ty}, but was {type(sym)}")
            return sym

    def assertExpectAnnotation(this, sym:cx_ast.ASTNode, selector:typing.Callable[[cx_ast.Annotation], bool], count:int):
        matches = []
        for i in sym.children:
            if isinstance(i, cx_ast.Annotation) and selector(i):
                matches.append(i)
        this.assertEqual(len(matches), count, f"Expected {count} annotations, but found {len(matches)}")
        return matches

    def test_basic_symbols_exist(this):
        this.assertExpectSymbol([CallParams("globalFunc", [ty_int, ty_char, ty_const_void_ptr])], cx_ast.GlobalFuncInfo)
        sym = this.assertExpectSymbol(["globalVarDefined"], cx_ast.GlobalVarInfo)
        this.assertIsNotNone(sym.definitionLocation, msg="Symbol should be defined")
        sym = this.assertExpectSymbol(["globalVarExterned"], cx_ast.GlobalVarInfo)
        this.assertIsNone(sym.definitionLocation, msg="Symbol should not be defined")
        sym = this.assertExpectSymbol(["globalVarExternedDefined"], cx_ast.GlobalVarInfo)
        this.assertIsNotNone(sym.definitionLocation, msg="Symbol should be defined")
        this.assertEqual(len(sym.declarationLocations), 1, msg="Symbol should be declared separate from definition")
        this.assertNotEqual(sym.declarationLocations[0], sym.definitionLocation, msg="Symbol should be declared separate from definition")
        
        this.assertExpectSymbol(["MyClass"], cx_ast.StructInfo)
        this.assertExpectSymbol(["MyClass", CallParams("MyClass", [])], cx_ast.ConstructorInfo)
        this.assertExpectSymbol(["MyClass", CallParams("~MyClass", [])], cx_ast.DestructorInfo)
        this.assertExpectSymbol(["MyClass", CallParams("MyClass", [ty_MyClass_const_ref])], cx_ast.ConstructorInfo)
        this.assertExpectSymbol(["MyClass", "foo"], cx_ast.FieldInfo)
        this.assertExpectSymbol(["MyClass", "bar"], cx_ast.FieldInfo)
        sym = this.assertExpectSymbol(["MyClass", "staticVarDefined"], cx_ast.StaticVarInfo)
        this.assertTrue(len(sym.declarationLocations) != 0 and sym.definitionLocation != None)
        this.assertTrue(sym.declarationLocations[0] != sym.definitionLocation)
        sym = this.assertExpectSymbol(["MyClass", "staticVarUndefined"], cx_ast.StaticVarInfo)
        this.assertTrue(len(sym.declarationLocations) != 0 and sym.definitionLocation == None)
        this.assertExpectSymbol(["MyClass", CallParams("myClassFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertExpectSymbol(["MyClass", CallParams("myConstClassFunc", [ty_int], ["const"])], cx_ast.MemFuncInfo) # TODO test disambiguation with this-const overloading
        this.assertExpectSymbol(["MyClass", CallParams("myStaticClassFunc", [ty_int, ty_MyClass_ptr])], cx_ast.StaticFuncInfo)
        
        mySubclass = this.assertExpectSymbol(["MySubclass"], cx_ast.StructInfo)
        parents = [i for i in mySubclass.children if isinstance(i, cx_ast.ParentInfo)]
        this.assertTrue(len(parents) == 1)
        this.assertTrue(parents[0].parentTypePath == cx_ast.SymbolPath()+"MyClass")
        
    def test_implicit_symbols_exist(this):
        this.assertExpectSymbol(["NonDefaulted"], cx_ast.StructInfo)
        this.assertExpectSymbol(["NonDefaulted", CallParams("NonDefaulted", [])], None) # Explicit ctor defined, removes implicit default ctor
        this.assertExpectSymbol(["NonDefaulted", CallParams("NonDefaulted", [ty_int])], cx_ast.ConstructorInfo)
        this.assertExpectSymbol(["NonDefaulted", CallParams("~NonDefaulted", [])], cx_ast.DestructorInfo) # Implicit dtor

    def test_virtual_method_detection(this):
        # Not virtual
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MyClass", CallParams("myClassFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertFalse(func.isVirtual)
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MyClass", CallParams("myConstClassFunc", [ty_int], ["const"])], cx_ast.MemFuncInfo)
        this.assertFalse(func.isVirtual)
        
        # Explicitly virtual
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MyClass", CallParams("myVirtualFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertTrue(func.isVirtual)
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MyClass", CallParams("myPureVirtualFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertTrue(func.isVirtual)
        
        # Implicitly virtual
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MySubclass", CallParams("myVirtualFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertTrue(func.isVirtual)
        func:cx_ast.MemFuncInfo = this.assertExpectSymbol(["MySubclass", CallParams("myPureVirtualFunc", [ty_int])], cx_ast.MemFuncInfo)
        this.assertTrue(func.isVirtual)
        
    def test_virtual_inheritance_detection(this):
        base      :cx_ast.StructInfo = this.assertExpectSymbol(["DiamondSharedBase"], cx_ast.StructInfo)
        a         :cx_ast.StructInfo = this.assertExpectSymbol(["DiamondA"], cx_ast.StructInfo)
        b         :cx_ast.StructInfo = this.assertExpectSymbol(["DiamondB"], cx_ast.StructInfo)
        grandchild:cx_ast.StructInfo = this.assertExpectSymbol(["DiamondGrandchild"], cx_ast.StructInfo)
        
        base_in_a = next((i for i in a.immediateParents if i.parentType==base))
        base_in_b = next((i for i in b.immediateParents if i.parentType==base))
        a_in_grandchild = next((i for i in grandchild.immediateParents if i.parentType==a))
        b_in_grandchild = next((i for i in grandchild.immediateParents if i.parentType==b))
        
        this.assertTrue(base_in_a.explicitlyVirtual)
        this.assertTrue(base_in_b.explicitlyVirtual)
        this.assertTrue(a_in_grandchild.explicitlyVirtual)
        this.assertTrue(b_in_grandchild.explicitlyVirtual)

    def test_visibility_detection(this):
        # ClassVisibilityTester
        sym:cx_ast.Member = this.assertExpectSymbol(["ClassVisibilityTester", "myDefault"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Private)
        sym:cx_ast.Member = this.assertExpectSymbol(["ClassVisibilityTester", "myPrivate"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Private)
        sym:cx_ast.Member = this.assertExpectSymbol(["ClassVisibilityTester", "myProtected"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Protected)
        sym:cx_ast.Member = this.assertExpectSymbol(["ClassVisibilityTester", "myPublic"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Public)
        
        # StructVisibilityTester
        sym:cx_ast.Member = this.assertExpectSymbol(["StructVisibilityTester", "myDefault"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Public)
        sym:cx_ast.Member = this.assertExpectSymbol(["StructVisibilityTester", "myPrivate"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Private)
        sym:cx_ast.Member = this.assertExpectSymbol(["StructVisibilityTester", "myProtected"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Protected)
        sym:cx_ast.Member = this.assertExpectSymbol(["StructVisibilityTester", "myPublic"], cx_ast.Member)
        this.assertTrue(sym.visibility == cx_ast.Member.Visibility.Public)

    def test_namespaced_exist(this):
        this.assertExpectSymbol(["MyNamespace"], cx_ast.Namespace)
        
        this.assertExpectSymbol(["MyNamespace", CallParams("globalFuncInNamespace", [ty_int, ty_char, ty_const_void_ptr])], cx_ast.GlobalFuncInfo)
        
        sym = this.assertExpectSymbol(["MyNamespace", "globalVarInNSDefined"], cx_ast.GlobalVarInfo)
        this.assertIsNotNone(sym.definitionLocation, msg="Symbol should be defined")
        sym = this.assertExpectSymbol(["MyNamespace", "globalVarInNSExterned"], cx_ast.GlobalVarInfo)
        this.assertIsNone(sym.definitionLocation, msg="Symbol should not be defined")
        
        this.assertExpectSymbol(["MyNamespace", "ClassInNamespace"], cx_ast.StructInfo)
        this.assertExpectSymbol(["MyNamespace", "ClassInNamespace", CallParams("ClassInNamespace", [])], cx_ast.ConstructorInfo) # Implicit default ctor
        this.assertExpectSymbol(["MyNamespace", "ClassInNamespace", CallParams("~ClassInNamespace", [])], cx_ast.DestructorInfo) # Implicit default dtor
        
    def test_annotations_exist(this):
        annotTgt = this.assertExpectSymbol([CallParams("annotatedGlobalFunc", [])], cx_ast.GlobalFuncInfo)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_globfunc", 1)
        annotTgt = this.assertExpectSymbol(["annotatedGlobalVar"], cx_ast.GlobalVarInfo)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_globvar", 1)
        annotTgt = this.assertExpectSymbol(["AnnotatedClass"], cx_ast.StructInfo)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_cls", 1)
        annotTgt = this.assertExpectSymbol(["AnnotatedClass", "foo"], cx_ast.FieldInfo)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_field", 1)
        annotTgt = this.assertExpectSymbol(["AnnotatedClass", CallParams("annotatedMemFunc", [])], cx_ast.MemFuncInfo)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_memfunc", 1)
        annotTgt = this.assertExpectSymbol(["AnnotatedNamespaceA"], cx_ast.Namespace)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_ns_a", 1)
        annotTgt = this.assertExpectSymbol(["AnnotatedNamespaceB"], cx_ast.Namespace)
        this.assertExpectAnnotation(annotTgt, lambda a: a.text == "annot_ns_b", 2)
        
    def test_templated_types_exist(this):
        tgt = this.assertExpectSymbol(["TemplatedType_Typename"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "typename" and tgt.paramName == "T" and tgt.defaultValue == None)

        tgt = this.assertExpectSymbol(["TemplatedType_Class"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "class" and tgt.paramName == "T" and tgt.defaultValue == None)
        
        tgt = this.assertExpectSymbol(["TemplatedType_Int"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "int" and tgt.paramName == "val" and tgt.defaultValue == None)

        tgt = this.assertExpectSymbol(["TemplatedType_VoidPtr"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "void*" and tgt.paramName == "val" and tgt.defaultValue == None)

        tgt = this.assertExpectSymbol(["TemplatedType_DefaultType"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "typename" and tgt.paramName == "T" and tgt.defaultValue == "int")

        tgt = this.assertExpectSymbol(["TemplatedType_DefaultInt"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "int" and tgt.paramName == "val" and tgt.defaultValue == "3")
        
        tgt = this.assertExpectSymbol(["TemplatedType_NamelessParam"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "typename" and tgt.paramName == "" and tgt.defaultValue == None)
        
        tgt = this.assertExpectSymbol(["TemplatedType_NamelessDefaultedParam"], cx_ast.StructInfo)
        tgt = next(( i for i in tgt.children if isinstance(i, cx_ast.TemplateParameter) ), None)
        this.assertTrue(tgt != None)
        this.assertTrue(tgt.paramType == "typename" and tgt.paramName == "" and tgt.defaultValue == "int")
        
        
from cx_ast_clang_reader import ClangParseContext
class TestClangParser(TestParser, unittest.TestCase):
    def invoke_parser(target:str, includes:list[str]):
        stixPath = os.path.dirname(__file__)
        stixPath = stixPath.replace("/", os.path.sep)
        assert os.path.exists(stixPath)
        args = [
            "--verbose",
            "--target" , f"{stixPath}/{target}"                   .replace("/", os.path.sep),
            "--output" , f"{stixPath}/tools/test_tmp/ast.stix-ast".replace("/", os.path.sep), # We'll never read/write this
            "--define" , "PLATFORM_DLL_EXTENSION=\".dll\";CS_ARCH_OURS=CS_ARCH_X86;NOMINMAX"
        ]
        if len(includes) > 0:
            args.append("--include")
            args.append(';'.join([f"{stixPath}/{i}" for i in includes]).replace("/", os.path.sep))
        
        arg_parser = argparse.ArgumentParser()
        ClangParseContext.argparser_add_defaults(arg_parser)
        args = arg_parser.parse_args(args)

        cx_project = source_discovery.Project(
            cx_ast_tooling.reduce_lists(args.targets),
            cx_ast_tooling.reduce_lists(args.includes)
        )
        cx_parser = ClangParseContext(cx_project, args, 0)
        cx_parser.configure()
        cx_parser.ingest()
        return cx_parser.module


if __name__ == '__main__':
    unittest.defaultTestLoader.sortTestMethodsUsing = lambda *args: -1 # Execute tests in the order they're declared
    unittest.main()
