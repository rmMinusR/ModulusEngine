#include "SAny.hpp"

#include <cassert>

void* stix::SAnyRef::get_internal(const TypeName& asType) const
{
	if (!data) return nullptr;

	assert(asType == type);
	return data;
}

void* stix::SAnyRef::getUnsafe() const
{
	return data;
}

stix::SAnyRef::SAnyRef(void* data, const TypeName& type) :
	data(data),
	type(type)
{
	assert(data);
	assert(type.isValid());
}

stix::SAnyRef::SAnyRef() :
	data(nullptr),
	type()
{
}

stix::SAnyRef::~SAnyRef()
{
}

TypeName stix::SAnyRef::getType() const
{
	return type;
}

stix::SAnyRef::operator bool() const
{
	return data;

}

bool stix::SAnyRef::has_value() const
{
	return data;
}
