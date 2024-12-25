#pragma once

struct GrandparentBase
{
	virtual ~GrandparentBase() = default;
};

struct IFoo
{
	virtual ~IFoo() = default;
	virtual int foo() = 0;
};

struct IBar
{
	virtual ~IBar() = default;
	virtual int bar() = 0;
};

struct ConcreteBase : public GrandparentBase
{
	char a; //Let's see what padding shows up...
	virtual ~ConcreteBase();
};

struct ImplementerA : public ConcreteBase, public IFoo, public IBar
{
	static int numDtorCalls;

	ImplementerA();
	virtual ~ImplementerA();

	virtual int foo() override;
	virtual int bar() override;
};

struct ImplementerB : public ConcreteBase, public IFoo, public IBar
{
	static int numDtorCalls;

	ImplementerB();
	virtual ~ImplementerB();

	virtual int foo() override;
	virtual int bar() override;
};
