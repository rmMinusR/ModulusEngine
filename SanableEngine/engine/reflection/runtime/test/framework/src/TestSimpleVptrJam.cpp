#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "MemberInfo.hpp"
#include "EmittedRTTI.hpp"

#include "Inheritance.hpp"

TEST_CASE("Implicit constant capture (simple)")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	//Setup
	const TypeInfo* td1 = TypeName::create<Derived1>().resolve();
	const TypeInfo* td2 = TypeName::create<Derived2>().resolve();
	REQUIRE(td1 != nullptr);
	REQUIRE(td2 != nullptr);

	Base* obj = new Derived1();
	CHECK(obj->identify() == Derived1::identify_s()); //Sanity

	td2->layout.vptrJam(obj); //Transmute
	CHECK(obj->identify() == Derived2::identify_s());

	td1->layout.vptrJam(obj); //Transmute back
	CHECK(obj->identify() == Derived1::identify_s());

	delete obj;
}
