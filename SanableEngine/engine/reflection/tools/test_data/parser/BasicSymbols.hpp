#pragma once

void globalFunc(int a, char b, const void* c);

class MyClass
{
	MyClass();
	virtual ~MyClass();

	MyClass(const MyClass& cpy) = delete;

	int foo;
	char bar;

	void myClassFunc(int param);
	const int* myConstClassFunc(int param) const;
	static void myStaticClassFunc(int param, MyClass* cls);

public:
	virtual void myVirtualFunc(int param);
	virtual void myPureVirtualFunc(int param) = 0;
};

class MySubclass : public MyClass
{
public:
	void myVirtualFunc(int param);
	void myPureVirtualFunc(int param) override;
};

class NonDefaulted
{
	NonDefaulted(int foo);
};

namespace MyNamespace
{
	void globalFuncInNamespace(int a, char b, const void* c);
	class ClassInNamespace {};
}

[[clang::annotate("annot_globfunc")]] void annotatedGlobalFunc();
class [[clang::annotate("annot_cls")]] AnnotatedClass
{
	[[clang::annotate("annot_field")]] int foo;
	[[clang::annotate("annot_memfunc")]] void annotatedMemFunc();
};
