#pragma once

#include <string>

struct Base
{
	virtual ~Base();
	virtual std::string identify() const = 0;
};

struct Derived1 : public Base
{
	virtual std::string identify() const override;
	static std::string identify_s();
};

struct Derived2 : public Base
{
	virtual std::string identify() const override;
	static std::string identify_s();
};

struct GrandchildOfBase : public Derived2
{
	virtual std::string identify() const override;
	static std::string identify_s();
};
