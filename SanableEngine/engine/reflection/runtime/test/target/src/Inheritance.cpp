#include "Inheritance.hpp"

Base::~Base()
{
}



std::string Derived1::identify() const
{
	return Derived1::identify_s();
}

std::string Derived1::identify_s()
{
	return "Derived1";
}

std::string Derived2::identify() const
{
	return Derived2::identify_s();
}

std::string Derived2::identify_s()
{
	return "Derived2";
}

std::string GrandchildOfBase::identify() const
{
	return GrandchildOfBase::identify_s();
}

std::string GrandchildOfBase::identify_s()
{
	return "GrandchildOfBase";
}
