#pragma once

#include <string>
#include <typeinfo>
#include <optional>
#include <vector>

#include "dllapi.h"
#include "StaticTemplateUtils.inl"

struct TypeInfo;
class SyntheticTypeBuilder;
class ModuleTypeRegistry;

namespace stix::detail
{
	template<bool _en> struct TypeName_staticEqualsDynamic_impl;
	template<bool _useIncompleteLiteral> struct TypeName_tryCreate_impl;
}

class TypeName
{
private:
	//Strip functions return true if any changes were made to str
	static bool strip(std::string& str, const std::string& phrase, bool spacePadded);
	static bool strip_leading(std::string& str, const std::string& phrase);
	static bool strip_trailing(std::string& str, const std::string& phrase);

	typedef uint32_t hash_t;
	static hash_t makeHash(const std::string& str);

	std::string name;
	hash_t nameHash;
	friend struct std::hash<TypeName>;

	enum Flags : uint8_t
	{
		Normal     = 0,
		Incomplete = 1<<0,
		Synthetic = 1<<1
	} flags;
	static const char* incomplete_ref_literal;
	STIX_API TypeName(const std::string& name, Flags flags);
public:
	STIX_API TypeName();
	STIX_API static TypeName incomplete_ref();

	template<typename TRaw>
	static TypeName create()
	{
		return TypeName(typeid(TRaw).name(), Flags::Normal);
	}

	template<typename... TPack>
	static std::vector<TypeName> createPack() { return { create<TPack>()... }; }

	STIX_API static TypeName createSynthetic(const std::string& name);

	template<typename T>
	static TypeName tryCreate()
	{
		return stix::detail::TypeName_tryCreate_impl<
			!stix::detail::is_complete_v<T> && std::is_reference_v<T>
		>::template exec<T>();
	}

	template<typename... TPack>
	static std::vector<TypeName> tryCreatePack() { return { tryCreate<TPack>()... }; }

	STIX_API std::optional<TypeName> cvUnwrap() const;
	STIX_API std::optional<TypeName> dereference() const;
	STIX_API bool isSynthetic() const;

	STIX_API bool isValid() const; //Whether the name has a valid value. Does NOT indicate whether there is live type data backing it.
	STIX_API TypeInfo const* resolve(ModuleTypeRegistry* moduleHint = nullptr) const;

	STIX_API bool operator==(const TypeName& other) const;
	STIX_API bool operator!=(const TypeName& other) const;

	STIX_API const std::string& as_str() const;
	STIX_API char const* c_str() const;

	STIX_API TypeName(const TypeName& cpy) = default;
	STIX_API TypeName(TypeName&& mov) = default;
	STIX_API TypeName& operator=(const TypeName& cpy) = default;
	STIX_API TypeName& operator=(TypeName&& mov) = default;
	STIX_API ~TypeName() = default;


	#pragma region Advanced template utils

	template<typename T, bool ignoreIncomplete>
	static bool staticEqualsDynamic(const TypeName& ty)
	{
		constexpr bool _do = !ignoreIncomplete || stix::detail::is_complete<T>::value;
		return stix::detail::TypeName_staticEqualsDynamic_impl<_do>::template eval<T>(ty);
	}

	template<typename It, bool ignoreIncomplete, typename THead, typename... Ts>
	static bool staticEqualsDynamic_many(It it, const It& end)
	{
		return it != end
			&& staticEqualsDynamic<THead, ignoreIncomplete>(stix::detail::_getRepresentedType(*it))
			&& staticEqualsDynamic_many<It, ignoreIncomplete, Ts...>(it+1, end);
	}
	template<typename It, bool ignoreIncomplete> static bool staticEqualsDynamic_many(It it, const It& end) { return it == end; } //Tail case
	
	#pragma endregion
};


namespace stix::detail
{
	//TODO move this to inside TypeName, if possible
	template<> struct TypeName_staticEqualsDynamic_impl<false>
	{
		template<typename T>
		static bool eval(const TypeName& ty) { return true; } // Incomplete type: can't check
	};
	template<> struct TypeName_staticEqualsDynamic_impl<true>
	{
		template<typename T>
		static bool eval(const TypeName& ty) { return ty == TypeName::create<T>(); }
	};


	template<> struct TypeName_tryCreate_impl<false>
	{
		template<typename T>
		static TypeName exec() { return TypeName::create<T>(); }
	};
	template<> struct TypeName_tryCreate_impl<true>
	{
		template<typename T>
		static TypeName exec() { return TypeName::incomplete_ref(); }
	};


	static inline decltype(auto) _getRepresentedType(const TypeName& t) { return t; }
}



template <>
struct std::hash<TypeName>
{
	STIX_API std::size_t operator()(const TypeName& k) const
	{
		return k.nameHash;
	}
};
