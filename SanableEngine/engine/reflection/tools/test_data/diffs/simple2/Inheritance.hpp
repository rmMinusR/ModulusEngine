#pragma once

#include <string>

struct Base
{
	virtual ~Base();
	virtual std::string identify() const = 0;
};

struct Derived : public Base
{
	virtual std::string identify() const override;
	static std::string identify_s();
};

struct AltDerived : public Base
{
	virtual std::string identify() const override;
	static std::string identify_s();
};

struct GrandchildOfBase : public Derived
{
	virtual std::string identify() const override;
	static std::string identify_s();
};
