#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "ThunkUtils.hpp"
#include "EmittedRTTI.hpp"

#include "Inheritance.hpp"
#include "MultiInheritance.hpp"
#include "VirtualInheritance.hpp"
#include "TypeBuilder.hpp"

template<typename T>
void testCtorCaptureV2();

TEST_CASE("Class image capture")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	SUBCASE("Simple case")
	{
		SUBCASE("Derived1") { testCtorCaptureV2<Derived1>(); }
		SUBCASE("Derived2") { testCtorCaptureV2<Derived2>(); }
		SUBCASE("GrandchildOfBase") { testCtorCaptureV2<GrandchildOfBase>(); }
	}

	SUBCASE("Multiple inheritance case")
	{
		//Complex case: Multiple inheritance
		SUBCASE("ConcreteBase") { testCtorCaptureV2<ConcreteBase>(); }
		SUBCASE("ImplementerA") { testCtorCaptureV2<ImplementerA>(); }
		SUBCASE("ImplementerB") { testCtorCaptureV2<ImplementerB>(); }
	}

	SUBCASE("Virtual inheritance case")
	{
		//Complex-er case: Multiple virtual inheritance
		SUBCASE("VirtualSharedBase") { testCtorCaptureV2<VirtualSharedBase>(); }
		SUBCASE("VirtualInheritedA") { testCtorCaptureV2<VirtualInheritedA>(); }
		SUBCASE("VirtualInheritedB") { testCtorCaptureV2<VirtualInheritedB>(); }
		SUBCASE("VirtualDiamond"   ) { testCtorCaptureV2<VirtualDiamond   >(); }
	}
}

template<typename T>
void testCtorCaptureV2()
{
	SUBCASE("Parity: not empty")
	{
		DetectedConstants vtables = thunk_utils<T>::template analyzeConstructor<>();
	
		size_t vtableByteCount = 0;
		for (size_t i = 0; i < sizeof(T); ++i) if (vtables.usage[i]) ++vtableByteCount;
	
		CHECK( std::is_polymorphic<T>::value == bool(vtableByteCount) );
		CHECK( (vtableByteCount%sizeof(void*)) == 0 );
	}

	SUBCASE("Parity vs self")
	{
		//printf("============== BEGIN %s v2/v2 parity check ==============\n\n", typeid(T).name());

		DetectedConstants vtables1 = thunk_utils<T>::template analyzeConstructor<>();
		DetectedConstants vtables2 = thunk_utils<T>::template analyzeConstructor<>();

		//Verify consistent
		bool good = true;
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			good &= (vtables1.usage[i] == vtables2.usage[i]);
			good &= (vtables1.bytes[i] == vtables2.bytes[i]);
		}
		CHECK(good);

		//printf("============== END %s v2/v2 parity check ==============\n\n", typeid(T).name());
	}

	SUBCASE("Parity vs v1")
	{
		for (uint8_t fill : { 0x00, 0x88, 0xFF })
		{
			//printf("============== BEGIN %s v1/v2 parity check ==============\n\n", typeid(T).name());
			const TypeInfo* ty = TypeName::create<T>().resolve();

			//Get baseline from old version
			uint8_t dummy[sizeof(T)];
			memset(dummy, fill, sizeof(T));
			ty->layout.vptrJam(dummy);

			//New version
			bool wasPreZeroed = true; //TODO detect pre-zeroing
			DetectedConstants vtables = thunk_utils<T>::template analyzeConstructor<>();

			//Unmark explicit fields
			ty->layout.walkFields(
				[&](const FieldInfo& fi)
				{
					for (int i = 0; i < fi.size; ++i) vtables.usage[fi.offset + i] = false;
				}
			);
			
			//*
			printf("v1 vptrs: ");
			for (size_t i = 0; i < sizeof(T); ++i) printf("%02x ", (uint32_t)dummy[i]);
			printf("\n");

			printf("v2 vptrs: ");
			for (size_t i = 0; i < sizeof(T); ++i) if (vtables.usage[i]) printf("%02x ", (uint32_t)vtables.bytes[i]); else printf(".. ");
			printf("\n");
			// */
			
			//Verify consistent
			bool good = true;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				if (vtables.usage[i]) good &= (dummy[i] == vtables.bytes[i]);
				else good &= (dummy[i] == fill);
			}
			CHECK(good);

			//printf("============== END %s v1/v2 parity check ==============\n\n", typeid(T).name());

			if (wasPreZeroed) return; //Cannot test against varying fill if pre-zeroing is happening
		}
	}
}
