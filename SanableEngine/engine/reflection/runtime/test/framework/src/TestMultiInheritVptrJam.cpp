#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "MemberInfo.hpp"
#include "EmittedRTTI.hpp"

#include "MultiInheritance.hpp"

TEST_CASE("Implicit constant capture (multiple inheritance)")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	//Setup
	const TypeInfo* ta = TypeName::create<ImplementerA>().resolve();
	const TypeInfo* tb = TypeName::create<ImplementerB>().resolve();
	REQUIRE(ta != nullptr);
	REQUIRE(tb != nullptr);

	SUBCASE("Transmuting ImplementerA => ImplementerB")
	{
		ImplementerA* obj = new ImplementerA();
		//NOTE: we forcibly change its type so this is a massive lie we're telling the compiler

		//Default state
		CHECK(obj->a == 1);
		CHECK(obj->foo() == 1);
		CHECK(obj->bar() == 2);

		tb->layout.vptrJam(obj); //Transmute

		CHECK(obj->a == 1); //Didn't change, since it was already initted
		CHECK(obj->foo() == 3); //Changed since it's tied to vptr
		CHECK(obj->bar() == 4);

		//Clean up, using DIFFERENT dtor than we started with!
		ImplementerA::numDtorCalls = 0;
		ImplementerB::numDtorCalls = 0;
		delete obj;
		CHECK(ImplementerA::numDtorCalls == 0);
		CHECK(ImplementerB::numDtorCalls == 1);
	}

	//Check the inverse, just to be sure
	SUBCASE("Transmuting ImplementerB => ImplementerA")
	{
		ImplementerB* obj = new ImplementerB();
		//NOTE: we forcibly change its type so this is a massive lie we're telling the compiler

		//Default state
		CHECK(obj->a == 2);
		CHECK(obj->foo() == 3);
		CHECK(obj->bar() == 4);

		ta->layout.vptrJam(obj); //Transmute

		CHECK(obj->a == 2); //Didn't change, since it was already initted
		CHECK(obj->foo() == 1); //Changed since it's tied to vptr
		CHECK(obj->bar() == 2);

		//Clean up, using DIFFERENT dtor than we started with!
		ImplementerA::numDtorCalls = 0;
		ImplementerB::numDtorCalls = 0;
		delete obj;
		CHECK(ImplementerA::numDtorCalls == 1);
		CHECK(ImplementerB::numDtorCalls == 0);
	}
}
