#pragma once

#include <utility>

#include "dllapi.h"
#include "DetectedConstants.hpp"

typedef void (*dtor_t)(void*); //CANNOT be a std::function or lambda because destroying the dtor_t instance would attempt to delete memory from a possibly-unloaded plugin


ENGINE_RTTI_API ptrdiff_t _captureCastOffset(const DetectedConstants& image, void*(*castThunk)(void*)); //TODO implement
ENGINE_RTTI_API DetectedConstants _captureVtablesInternal(size_t objSize, void(*thunk)(), size_t destructorCallCount, const std::vector<void(*)()>& allocators, const std::vector<void(*)()>& nofill);


template<typename T>
struct thunk_utils
{
	thunk_utils() = delete;
	
	template<typename... Args>
	static void thunk_newInPlace(T* obj, Args... args) { new(obj) T(std::forward<Args>(args)...); }

private:
	//Helper for SemanticVM ThisPtr detection. Dllimport'ed functions such as malloc may have
	//separate callsite addresses per translation unit (even if deferring to a shared libc)
	template<int> static void* dummyAllocator() { return nullptr; }

	template<typename... Args>
	static void _simulated_thunk_newInPlace(Args... args) { new(dummyAllocator<0>()) T(std::forward<Args>(args)...); }

	template<typename _Ty>
	constexpr static bool requires_destruction = std::is_destructible_v<_Ty> && !std::is_trivially_destructible_v<_Ty> && !std::is_reference_v<_Ty>;
public:
	template<typename... Args>
	static inline DetectedConstants analyzeConstructor()
	{
		return _captureVtablesInternal(
			sizeof(T),
			(void(*)()) &_simulated_thunk_newInPlace<Args...>,
			( size_t(requires_destruction<Args>) + ... + 0 ),
			{
				(void(*)()) &dummyAllocator<0>,
				(void(*)()) &malloc,
				(void(*)()) (void*(*)(size_t)) &::operator new
				//TODO: Capture from inherited TUs?
			},
			{ (void(*)()) &memset } //Some compilers will pre-zero, especially in debug mode. Don't catch that. &memset will be unique per-module, so we need to pass this per calling TU.
		);
	}
		
	#pragma region Destructor

	template<bool has_destructor>
	struct _dtor { _dtor() = delete; };

	template<>
	struct _dtor<true>
	{
		//C++ forbids getting the address of a dtor, but we can still wrap it
		inline static void call_dtor(void* obj) { static_cast<const T*>(obj)->~T(); }

		constexpr static dtor_t dtor = &call_dtor;
	};

	template<>
	struct _dtor<false>
	{
		//Can't call a dtor that doesn't exist
		constexpr static dtor_t dtor = nullptr;
	};

	constexpr static dtor_t dtor = ::thunk_utils<T>::_dtor<std::is_destructible<T>::value>::dtor;

	#pragma endregion
};
