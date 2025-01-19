#pragma once

#include <type_traits>

#include "TypeName.hpp"

namespace stix
{

	class MemberFunction;
	class StaticFunction;

	//Pass objects by reference while also erasing their type. Similar to Object in Java/C#/Python.
	//Does not own the given object; should never be saved in an object.
	class SAnyRef
	{
		void* data;
		TypeName type;

		friend class ::stix::MemberFunction;
		friend class ::stix::StaticFunction;

		STIX_API void* get_internal(const TypeName& asType) const;
		STIX_API void* get_unchecked() const;
		STIX_API SAnyRef(void* data, const TypeName& type);
	public:
		STIX_API SAnyRef();
		STIX_API ~SAnyRef();

		template<typename T>
		static SAnyRef make(T* obj)
		{
			static_assert(!std::is_reference_v<T>);
			return SAnyRef(obj, TypeName::create<T>()); //TODO attempt to snipe RTTI and checK, just to be sure? Casting should handle most of it though.
		}

		STIX_API TypeName getType() const;
		template<typename T>
		std::remove_reference_t<T>& get() const { return *(std::remove_reference_t<T>*)get_internal(TypeName::tryCreate<T>()); }

		STIX_API operator bool() const;
		STIX_API bool has_value() const;

		//Allow copy and move
		SAnyRef(const SAnyRef& cpy) = default;
		SAnyRef& operator=(const SAnyRef& cpy) = default;
		SAnyRef(SAnyRef&& mov) = default;
		SAnyRef& operator=(SAnyRef&& mov) = default;
	};


	namespace detail
	{
		static inline decltype(auto) _getRepresentedType(const SAnyRef& v) { return v.getType(); }
	}
}
