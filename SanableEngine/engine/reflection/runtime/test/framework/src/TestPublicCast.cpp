#include <doctest/doctest.h>

#include <cstddef>
#include <memory>

#include <public_cast.hpp>

//Test rig class
class PublicCastTester
{
public:
	int pub;
private:
	int priv;
	static int privStatic;

	static int privateStaticFn(int arg1, int arg2)
	{
		return 123;
	}

	long privateMemFn(char arg1, void* arg2)
	{
		return 456;
	}

public:
	static ptrdiff_t builtin_offsetof_priv()
	{
		return offsetof(PublicCastTester, priv);
	}

	int* get_priv() { return &priv; }
	static int* get_privStatic() { return &privStatic; }

	static auto get_privateFn() { return &privateStaticFn; }
	static auto get_privateMemFn() { return &PublicCastTester::privateMemFn; }
};
int PublicCastTester::privStatic = 0;


//Test rig: grant access to members
PUBLIC_CAST_GIVE_FIELD_ACCESS     (PublicCastTester_priv           , PublicCastTester, priv           , int);
PUBLIC_CAST_GIVE_STATIC_VAR_ACCESS(PublicCastTester_privStatic     , PublicCastTester, privStatic     , int);
PUBLIC_CAST_GIVE_STATIC_FN_ACCESS (PublicCastTester_privateStaticFn, PublicCastTester, privateStaticFn, int, int, int);
PUBLIC_CAST_GIVE_BOUND_FN_ACCESS  (PublicCastTester_privateMemFn   , PublicCastTester, privateMemFn   , long, char, void*);

TEST_SUITE("public_cast")
{
	TEST_CASE("Member variables")
	{
		PublicCastTester tester;
		REQUIRE(&(tester.*DO_PUBLIC_CAST(PublicCastTester_priv)) == tester.get_priv());

		SUBCASE("Assignment")
		{
			tester.*DO_PUBLIC_CAST(PublicCastTester_priv) = 2;
			CHECK(*tester.get_priv() == 2);
			tester.*DO_PUBLIC_CAST(PublicCastTester_priv) = 5;
			CHECK(*tester.get_priv() == 5);
		}

		SUBCASE("Offsetof (manual, no instance)")
		{
			PublicCastTester* _tester = nullptr;
			ptrdiff_t diff = (char*)std::addressof(_tester->*DO_PUBLIC_CAST(PublicCastTester_priv)) - (char*)_tester;
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}

		SUBCASE("Offsetof (manual, corrupted instance)")
		{
			PublicCastTester* _tester = reinterpret_cast<PublicCastTester*>(0xDEADBEEF);
			ptrdiff_t diff = (char*)std::addressof(_tester->*DO_PUBLIC_CAST(PublicCastTester_priv)) - (char*)_tester;
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}
		
		SUBCASE("Offsetof (manual, concrete instance)")
		{
			ptrdiff_t diff = (char*)std::addressof(tester.*DO_PUBLIC_CAST(PublicCastTester_priv)) - (char*)&tester;
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}
		
		SUBCASE("Offsetof (macroed, no instance)")
		{
			ptrdiff_t diff = DO_PUBLIC_CAST_OFFSETOF_LAMBDA(PublicCastTester_priv, PublicCastTester)(nullptr);
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}

		SUBCASE("Offsetof (macroed, corrupted instance)")
		{
			PublicCastTester* _tester = reinterpret_cast<PublicCastTester*>(0xDEADBEEF);
			ptrdiff_t diff = DO_PUBLIC_CAST_OFFSETOF_LAMBDA(PublicCastTester_priv, PublicCastTester)(_tester);
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}

		SUBCASE("Offsetof (macroed, concrete instance)")
		{
			ptrdiff_t diff = DO_PUBLIC_CAST_OFFSETOF_LAMBDA(PublicCastTester_priv, PublicCastTester)(&tester);
			CHECK(diff == PublicCastTester::builtin_offsetof_priv());
		}
	}

	TEST_CASE("Static variables")
	{
		SUBCASE("Assignment")
		{
			REQUIRE(DO_PUBLIC_CAST(PublicCastTester_privStatic) == PublicCastTester::get_privStatic());
			*DO_PUBLIC_CAST(PublicCastTester_privStatic) = 2;
			CHECK(*PublicCastTester::get_privStatic() == 2);
			*DO_PUBLIC_CAST(PublicCastTester_privStatic) = 5;
			CHECK(*PublicCastTester::get_privStatic() == 5);
		}
	}
	
	TEST_CASE("Static function")
	{
		auto fnPtr = DO_PUBLIC_CAST(PublicCastTester_privateStaticFn);
		REQUIRE(fnPtr == PublicCastTester::get_privateFn());
		int res = fnPtr(3, 6);
		CHECK(res == 123);
	}

	TEST_CASE("Member function")
	{
		PublicCastTester tester;
		auto fp = DO_PUBLIC_CAST(PublicCastTester_privateMemFn);
		REQUIRE(fp == PublicCastTester::get_privateMemFn());
		int res = (tester.*fp)(1, nullptr);
		CHECK(res == 456);
	}
}
