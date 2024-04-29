#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cassert>

#include "TypeName.hpp"
#include "SAny.hpp"
#include "CallableUtils.inl"

namespace stix
{

	class Function
	{
	public:
		TypeName returnType;
		std::vector<TypeName> parameters;

		ENGINE_RTTI_API virtual ~Function();
	protected:
		Function(const TypeName& returnType, const std::vector<TypeName>& parameters);
	};

	class MemberFunction : public Function
	{
	public:
		ENGINE_RTTI_API virtual ~MemberFunction();
		ENGINE_RTTI_API void invoke(SAnyRef returnValue, const SAnyRef& thisObj, const std::vector<SAnyRef>& parameters) const;
	
		template<typename TReturn, typename TOwner, typename... TArgs> static MemberFunction make(TReturn(TOwner::* fn)(TArgs...)      ) { return make_internal(fn, false); }
		template<typename TReturn, typename TOwner, typename... TArgs> static MemberFunction make(TReturn(TOwner::* fn)(TArgs...) const) { return make_internal( (TReturn(TOwner::*)(TArgs...)) fn, true); }

	protected:
		ENGINE_RTTI_API MemberFunction(const TypeName& owner, bool ownerIsConst, const TypeName& returnType, const std::vector<TypeName>& parameters,
			                           detail::CallableUtils::Member::fully_erased_binder_t binder, detail::CallableUtils::Member::erased_fp_t fn);

		template<typename TReturn, typename TOwner, typename... TArgs>
		static MemberFunction make_internal(TReturn(TOwner::* fn)(TArgs...), bool ownerIsConst)
		{
			std::vector<TypeName> parameters = TypeName::createPack<TArgs...>();
			detail::CallableUtils::checkArgs<std::vector<TypeName>::const_iterator, TArgs...>(parameters.cbegin(), parameters.cend());
			detail::CallableUtils::Member::fully_erased_binder_t eraser = &detail::CallableUtils::Member::TypeEraser<TReturn>::template impl<TOwner, TArgs...>;

			static_assert(sizeof(detail::CallableUtils::Member::erased_fp_t) >= sizeof(fn));
			union //reinterpret_cast doesn't allow us to do this conversion. Too bad!
			{
				decltype(fn) _fn;
				detail::CallableUtils::Member::erased_fp_t _erased;
			} reinterpreter;
			reinterpreter._fn = fn;

			return MemberFunction(TypeName::create<TOwner>(), ownerIsConst, TypeName::create<TReturn>(), parameters, eraser, reinterpreter._erased);
		}

		//All SAnyRefs guaranteed valid when called
		TypeName owner;
		bool ownerIsConst; //TODO safety check on invoke
		detail::CallableUtils::Member::fully_erased_binder_t binder;
		detail::CallableUtils::Member::erased_fp_t fn;
	};

	class StaticFunction : public Function
	{
	public:
		ENGINE_RTTI_API virtual ~StaticFunction();
		ENGINE_RTTI_API void invoke(SAnyRef returnValue, const std::vector<SAnyRef>& parameters) const;

		template<typename TReturn, typename... TArgs>
		static StaticFunction make(TReturn(*fn)(TArgs...))
		{
			std::vector<TypeName> parameters = TypeName::createPack<TArgs...>();
			detail::CallableUtils::checkArgs<std::vector<TypeName>::const_iterator, TArgs...>(parameters.cbegin(), parameters.cend());
			auto eraser = &detail::CallableUtils::Static::TypeEraser<TReturn>::template impl<TArgs...>;
			return StaticFunction(
				TypeName::create<TReturn>(),
				parameters,
				(detail::CallableUtils::Static::fully_erased_binder_t) eraser,
				(detail::CallableUtils::Static::erased_fp_t) fn
			);
		}
	
	protected:
		ENGINE_RTTI_API StaticFunction(const TypeName& returnType, const std::vector<TypeName>& parameters,
			                           detail::CallableUtils::Static::fully_erased_binder_t binder, detail::CallableUtils::Static::erased_fp_t fn);

		//All SAnyRefs guaranteed valid when called
		detail::CallableUtils::Static::fully_erased_binder_t binder;
		detail::CallableUtils::Static::erased_fp_t fn;
	};

}
