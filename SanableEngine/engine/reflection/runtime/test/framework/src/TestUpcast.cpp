#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "MemberInfo.hpp"
#include "EmittedRTTI.hpp"

#include "MultiInheritance.hpp"
#include "SimpleStruct.hpp"

TEST_CASE("TypeInfo::upcast")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	const TypeInfo* ti = TypeName::create<ImplementerA>().resolve();
	REQUIRE(ti != nullptr);

	SUBCASE("Upcast to self")
	{
		ImplementerA obj;
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<ImplementerA>()) == nullptr);
	}

	SUBCASE("Upcast to immediate parents")
	{
		ImplementerA obj;
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<ConcreteBase>()) == (ConcreteBase*)&obj);
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<IFoo        >()) == (IFoo        *)&obj);
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<IBar        >()) == (IBar        *)&obj);
	}

	SUBCASE("Upcast to grandparent")
	{
		ImplementerA obj;
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<GrandparentBase>()) == (GrandparentBase*)&obj);
	}

	SUBCASE("Upcast to non-parent")
	{
		ImplementerA obj;
		REQUIRE(ti->layout.upcast(&obj, TypeName::create<SimpleStruct>()) == nullptr);
	}
}
