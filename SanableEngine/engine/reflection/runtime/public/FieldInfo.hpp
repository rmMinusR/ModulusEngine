#pragma once

#include "MemberInfo.hpp"
#include "TypeName.hpp"

struct FieldInfo : public MemberInfo
{
	TypeName type;
	std::string name;
	MemberVisibility visibility;

	FieldInfo(size_t size, ptrdiff_t offset, const TypeName& owner, const TypeName& type, const std::string& name, MemberVisibility visibility);

	STIX_API void* getValue(void* objInstance) const;
	STIX_API void blitValue(void* objInstance, void* value) const; //Does NOT call copy ctor! FIXME
};
