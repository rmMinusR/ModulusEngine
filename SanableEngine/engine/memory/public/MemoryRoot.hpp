#pragma once

#include <optional>
#include <tuple>
#include <map>
#include <set>
#include <functional>

#include <SAny.hpp>

#include "dllapi.h"

#include "ExternalObject.hpp"

class MemoryHeap;
class ObjectRelocator;

/// <summary>
/// The base root of a program's "managed" memory. There can only be one.
/// </summary>
class MemoryRoot
{
public:
	//FIXME make private
	ENGINEMEM_API MemoryRoot();
	ENGINEMEM_API ~MemoryRoot();

	ENGINEMEM_API static MemoryRoot* get();
	ENGINEMEM_API static void cleanup();
	
	ENGINEMEM_API void visitHeaps(const std::function<void(MemoryHeap*)>& visitor);
	ENGINEMEM_API void updatePointers(const ObjectRelocator& remapper, std::set<void*>& visitRecord);

	template<typename T>
	void registerExternal(T* object, ExternalObjectOptions options = ExternalObjectOptions::DefaultExternal)
	{
		registerExternal_impl(object, std::move(TypeName::create<T>()), sizeof(T), options);
	}
	ENGINEMEM_API void removeExternal(void* object);
	ENGINEMEM_API void visitExternals(const std::function<void(stix::SAnyRef, ExternalObjectOptions)>& visitor);

private:
	static std::optional<MemoryRoot> instance;

	std::vector<MemoryHeap*> livingHeaps;
	friend class MemoryHeap;
	void registerHeap(MemoryHeap* heap);
	void removeHeap(MemoryHeap* heap);

	std::map<void*, std::tuple<TypeName, size_t, ExternalObjectOptions>> externalObjects;
	ENGINEMEM_API void registerExternal_impl(void* object, TypeName&& type, size_t size, ExternalObjectOptions options);

	void externals_updatePointers(const ObjectRelocator& remapper, std::set<void*>& visitRecord);
};
