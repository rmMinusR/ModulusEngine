#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "MemberInfo.hpp"
#include "EmittedRTTI.hpp"

#include "VirtualInheritance.hpp"

TEST_CASE("TypeInfo::upcast (virtual)")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	SUBCASE("Direct upcast to virtual")
	{
		const TypeInfo* ti = TypeName::create<VirtualInheritedA>().resolve();
		REQUIRE(ti != nullptr);

		VirtualInheritedA obj;
		CHECK(ti->layout.upcast(&obj, TypeName::create<VirtualSharedBase>()) == (VirtualSharedBase*)&obj);
	}

	SUBCASE("Upcast to diamond virtual")
	{
		const TypeInfo* ti = TypeName::create<VirtualDiamond>().resolve();
		REQUIRE(ti != nullptr);

		VirtualDiamond obj;
		CHECK(ti->layout.upcast(&obj, TypeName::create<VirtualSharedBase>()) == (VirtualSharedBase*)&obj);
	}
}
