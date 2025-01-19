#include <doctest/doctest.h>

#include <GlobalTypeRegistry.hpp>
#include <TypeInfo.hpp>
#include "EmittedRTTI.hpp"
#include "VirtualInheritance.hpp"
#include "Callables.hpp"

TEST_CASE("Function capture")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	SUBCASE("Static")
	{
		SUBCASE("VirtualSharedBase::identify_s")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualSharedBase>());
			REQUIRE(t != nullptr);
			const stix::StaticFunction* fn = t->capabilities.getStaticFunction("identify_s");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), {});
			CHECK(result == VirtualSharedBase::identify_s());
		}

		SUBCASE("VirtualInheritedA::identify_s")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualInheritedA>());
			REQUIRE(t != nullptr);
			const stix::StaticFunction* fn = t->capabilities.getStaticFunction("identify_s");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), {});
			CHECK(result == VirtualInheritedA::identify_s());
		}

		SUBCASE("VirtualInheritedB::identify_s")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualInheritedB>());
			REQUIRE(t != nullptr);
			const stix::StaticFunction* fn = t->capabilities.getStaticFunction("identify_s");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualInheritedB obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), {});
			CHECK(result == VirtualInheritedB::identify_s());
		}
		
		SUBCASE("VirtualDiamond::identify_s")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualDiamond>());
			REQUIRE(t != nullptr);
			const stix::StaticFunction* fn = t->capabilities.getStaticFunction("identify_s");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualDiamond obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), {});
			CHECK(result == VirtualDiamond::identify_s());
		}
	}

	SUBCASE("Member")
	{
		SUBCASE("CallableStruct::myFunc1_thiscall")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<Callables::CallableStruct>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("myFunc1_thiscall");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<long>());
		}

		SUBCASE("CallableStruct::myFunc2_thiscall")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<Callables::CallableStruct>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("myFunc2_thiscall");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<void>());
		}

		SUBCASE("CallableStruct::myFunc3_thiscall")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<Callables::CallableStruct>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("myFunc3_thiscall");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 1);
			CHECK(fn->parameters[0] == TypeName::create<int>());
			CHECK(fn->returnType == TypeName::create<void>());
		}

		SUBCASE("CallableStruct::myFunc4_thiscall")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<Callables::CallableStruct>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("myFunc4_thiscall");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 2);
			CHECK(fn->parameters[0] == TypeName::create<int>());
			CHECK(fn->parameters[1] == TypeName::create<char>());
			CHECK(fn->returnType == TypeName::create<void>());
		}
	}

	SUBCASE("Virtual")
	{
		SUBCASE("VirtualSharedBase::identify")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualSharedBase>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("identify");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualSharedBase obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), {});
			CHECK(result == obj.identify());
		}

		SUBCASE("VirtualInheritedA::identify")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualInheritedA>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("identify");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualInheritedA obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), {});
			CHECK(result == obj.identify());
		}

		SUBCASE("VirtualInheritedB::identify")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualInheritedB>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("identify");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualInheritedB obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), {});
			CHECK(result == obj.identify());
		}
		
		SUBCASE("VirtualDiamond::identify")
		{
			const TypeInfo* t = GlobalTypeRegistry::lookupType(TypeName::create<VirtualDiamond>());
			REQUIRE(t != nullptr);
			const stix::MemberFunction* fn = t->capabilities.getMemberFunction("identify");
			REQUIRE(fn != nullptr);
			CHECK(fn->parameters.size() == 0);
			CHECK(fn->returnType == TypeName::create<std::string>());

			VirtualDiamond obj;
			std::string result;
			fn->invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), {});
			CHECK(result == obj.identify());
		}
	}
}
