#pragma once

#include <cstddef>

#include "dllapi.h"
#include "TypeName.hpp"

struct MemberInfo //TODO rename: Callables are members, but don't appear in object layout
{
	size_t size;
	ptrdiff_t offset;
	TypeName owner;

protected:
	MemberInfo(size_t size, ptrdiff_t offset, const TypeName& owner);
	STIX_API void* getAddr(void* objInstance) const;
};

enum class MemberVisibility : uint8_t
{
	Private   = 1<<0,
	Protected = 1<<1,
	Public    = 1<<2,

	All = 0b111,
	None = 0
};
inline constexpr MemberVisibility operator&(const MemberVisibility a, const MemberVisibility b) { return MemberVisibility( uint8_t(a)&uint8_t(b) ); }
inline constexpr MemberVisibility operator|(const MemberVisibility a, const MemberVisibility b) { return MemberVisibility( uint8_t(a)|uint8_t(b) ); }
