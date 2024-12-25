#include <doctest/doctest.h>

#include "TypeName.hpp"

TEST_CASE("Name unwrapping")
{
	SUBCASE("Non-pointer dereferencing")
	{
		TypeName name = TypeName::create<int>();
		std::optional<TypeName> unwrapped = name.dereference();
		REQUIRE(!unwrapped.has_value());
	}

	SUBCASE("Pointer dereferencing")
	{
		TypeName name = TypeName::create<int*>();
		std::optional<TypeName> unwrapped = name.dereference();
		REQUIRE(unwrapped.has_value());
		CHECK(unwrapped.value() == TypeName::create<int>());
	}

	SUBCASE("Unwrapping non-const value")
	{
		TypeName name = TypeName::create<int>();
		std::optional<TypeName> unwrapped = name.cvUnwrap();
		CHECK(!unwrapped.has_value());
	}

	SUBCASE("Unwrapping const value")
	{
		TypeName name = TypeName::create<int const>();
		std::optional<TypeName> unwrapped = name.cvUnwrap();
		CHECK(unwrapped.value_or(name) == TypeName::create<int>());
	}

	SUBCASE("Unwrapping const pointer")
	{
		TypeName name = TypeName::create<int* const>();
		std::optional<TypeName> unwrapped = name.cvUnwrap();
		CHECK(unwrapped.value_or(name) == TypeName::create<int*>());
	}

	SUBCASE("Unwrapping pointer-to-const")
	{
		TypeName name = TypeName::create<int const*>();
		std::optional<TypeName> unwrapped = name.cvUnwrap();
		CHECK(!unwrapped.has_value());
	}

	SUBCASE("Unwrapping const pointer-to-const")
	{
		TypeName name = TypeName::create<int const* const>();
		std::optional<TypeName> unwrapped = name.cvUnwrap();
		CHECK(unwrapped.value_or(name) == TypeName::create<int const*>());
	}
}
