#pragma once

#include "MemberInfo.hpp"
#include "TypeName.hpp"

struct ParentInfo : public MemberInfo
{
	TypeName typeName;
	MemberVisibility visibility;

	//Derived classes inherit all fields of non-virtual bases, embedded as if a field were explicitly declared.
	//For virtual bases, derived classes always embed a pointer or offset (implementation-defined). When classes with any virtual base
	//are explicitly referenced by the user, the compiler also embeds inherited fields in the final version, but these inherited fields
	//are omitted from all other bases that inherit that virtual base.
	enum class Virtualness
	{
		NonVirtual = 0b01,
		VirtualExplicit = 0b11,
		VirtualInherited = 0b10
	};

	Virtualness virtualness;

	ParentInfo(size_t size, ptrdiff_t offset, const TypeName& owner, const TypeName& typeName, MemberVisibility visibility, Virtualness virtualness);
	STIX_API static ParentInfo identity(const TypeName& ty, size_t sz);
};
