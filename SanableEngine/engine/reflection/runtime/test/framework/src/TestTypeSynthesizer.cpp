#include <doctest/doctest.h>

#include <algorithm>

#include "SyntheticTypeBuilder.hpp"

#define FOREACH_TEST_FIELD() \
	_X(char , myChar ) \
	_X(int  , myInt  ) \
	_X(short, myShort) \
	_X(long long, myLong64)

struct MyTypeImpl
{
#define _X(ty, name) ty name;
	FOREACH_TEST_FIELD()
#undef _X
};

template<typename T>
void checkFieldLocation(const TypeInfo& type, const std::string& name)
{
	const FieldInfo* fi = type.layout.getField(name);
	CHECK(fi->size == sizeof(T));
	CHECK((fi->offset % alignof(T)) == 0);
}

TEST_CASE("Type synthesis")
{
	//Arrange
	SyntheticTypeBuilder builder("MyType");
	#define _X(ty, name) builder.addField<ty>(#name);
	FOREACH_TEST_FIELD();
	#undef _X
		
	//Act
	TypeInfo type = builder.finalize();

	//Assert: fields
	#define _X(ty, name) checkFieldLocation<ty>(type, #name);
	FOREACH_TEST_FIELD();
	#undef _X

	//Assert: type
	#define _X(ty, name) alignof(ty),
	size_t expectedAlign = std::max({ FOREACH_TEST_FIELD() });
	CHECK(type.layout.align == expectedAlign);
	#undef _X
}
