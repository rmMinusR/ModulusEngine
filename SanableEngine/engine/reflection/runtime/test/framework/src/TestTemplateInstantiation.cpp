#include <doctest/doctest.h>

#include "GlobalTypeRegistry.hpp"
#include "EmittedRTTI.hpp"
#include "TemplateTypeInfo.hpp"

TEST_CASE("Template instantiation")
{
	//Prepare clean state
	{
		GlobalTypeRegistry::clear();
		GlobalTypeRegistry::loadIntrinsics();
		ModuleTypeRegistry m;
		plugin_reportTypes(&m);
		GlobalTypeRegistry::loadModule("test runner", m);
	}

	SUBCASE("1 static type field")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		ty.addField(TypeName::create<int>(), "bar");

		// Check template validity
		CHECK(ty.minSize() == sizeof(int));
		CHECK(ty.minAlign() == alignof(int));

		// Instantiate template and check instance validity
		TypeInfo t = ty.instantiate({ });
		CHECK(t.name.as_str() == "Foo<>");
		CHECK(t.layout.size == sizeof(int));
		CHECK(t.layout.align == alignof(int));
	}

	SUBCASE("1 template field")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		auto T = ty.addTypeParam("T");
		ty.addField(T, "bar");

		// Check template validity
		CHECK(ty.minSize() == 1);
		CHECK(ty.minAlign() == 1);

		// Instantiate template and check instance validity

		{
			using tested_t = int;
			TypeInfo t = ty.instantiate({ TypeName::create<tested_t>().resolve() });
			CHECK(t.name.as_str() == "Foo<int>");
			CHECK(t.layout.size == sizeof(tested_t));
			CHECK(t.layout.align == alignof(tested_t));
		}

		{
			using tested_t = char;
			TypeInfo t = ty.instantiate({ TypeName::create<tested_t>().resolve() });
			CHECK(t.name.as_str() == "Foo<char>");
			CHECK(t.layout.size == sizeof(tested_t));
			CHECK(t.layout.align == alignof(tested_t));
		}
	}

	SUBCASE("Interleaved fields")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		auto T = ty.addTypeParam("T");
		ty.addField(TypeName::create<int>(), "i1");
		ty.addField(T, "t1");
		ty.addField(TypeName::create<int>(), "i2");
		ty.addField(T, "t2");

		// Check template validity
		CHECK(ty.minSize() == sizeof(int)*3 + 1); // int, T, padding, int, T
		CHECK(ty.minAlign() == alignof(int));

		// Instantiate template and check instance validity
		TypeInfo t = ty.instantiate({ TypeName::create<char>().resolve() });
		CHECK(t.name.as_str() == "Foo<char>");
		CHECK(t.layout.size == sizeof(int)*3 + sizeof(char)); // int, char, padding, int, char
		CHECK(t.layout.align == std::max(alignof(int), alignof(char)));
	}

	SUBCASE("1 static parent, no fields")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		ty.addParent(TypeName::create<int>());

		// Check template validity
		CHECK(ty.minSize() == sizeof(int));
		CHECK(ty.minAlign() == alignof(int));

		// Instantiate template and check instance validity
		TypeInfo t = ty.instantiate({ });
		CHECK(t.name.as_str() == "Foo<>");
		CHECK(t.layout.size == sizeof(int));
		CHECK(t.layout.align == alignof(int));
	}

	SUBCASE("1 template parent, no fields")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		auto T = ty.addTypeParam("T");
		ty.addParent(T);

		// Check template validity
		CHECK(ty.minSize() == 1);
		CHECK(ty.minAlign() == 1);

		// Instantiate template and check instance validity
		{
			using tested_t = int;
			TypeInfo t = ty.instantiate({ TypeName::create<tested_t>().resolve() });
			CHECK(t.name.as_str() == "Foo<int>");
			CHECK(t.layout.size == sizeof(tested_t));
			CHECK(t.layout.align == alignof(tested_t));
		}

		{
			using tested_t = char;
			TypeInfo t = ty.instantiate({ TypeName::create<tested_t>().resolve() });
			CHECK(t.name.as_str() == "Foo<char>");
			CHECK(t.layout.size == sizeof(tested_t));
			CHECK(t.layout.align == alignof(tested_t));
		}
	}

	SUBCASE("Interleaved parents, no fields")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		auto T = ty.addTypeParam("T");
		ty.addParent(TypeName::create<int>());
		ty.addParent(T);
		ty.addParent(TypeName::create<int>());
		ty.addParent(T);

		// Check template validity
		CHECK(ty.minSize() == sizeof(int) * 3 + 1); // int, T, padding, int, T
		CHECK(ty.minAlign() == alignof(int));

		// Instantiate template and check instance validity
		TypeInfo t = ty.instantiate({ TypeName::create<char>().resolve() });
		CHECK(t.name.as_str() == "Foo<char>");
		CHECK(t.layout.size == sizeof(int) * 3 + sizeof(char)); // int, char, padding, int, char
		CHECK(t.layout.align == std::max(alignof(int), alignof(char)));
	}

	SUBCASE("Interleaved parents and fields")
	{
		// Assemble template
		TemplateTypeInfo ty("Foo");
		auto T = ty.addTypeParam("T");
		ty.addParent(TypeName::create<int>());
		ty.addParent(T);
		ty.addField(TypeName::create<int>(), "i");
		ty.addField(T, "t");

		// Check template validity
		CHECK(ty.minSize() == sizeof(int) * 3 + 1); // int, T, padding, int, T
		CHECK(ty.minAlign() == alignof(int));

		// Instantiate template and check instance validity
		TypeInfo t = ty.instantiate({ TypeName::create<char>().resolve() });
		CHECK(t.name.as_str() == "Foo<char>");
		CHECK(t.layout.size == sizeof(int) * 3 + sizeof(char)); // int, char, padding, int, char
		CHECK(t.layout.align == std::max(alignof(int), alignof(char)));
	}

}
