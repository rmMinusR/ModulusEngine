#include "MemoryRoot.hpp"

#include <cassert>

#include "MemoryHeap.hpp"

std::optional<MemoryRoot> MemoryRoot::instance;

MemoryRoot* MemoryRoot::get()
{
	if (!MemoryRoot::instance.has_value()) MemoryRoot::instance.emplace();
	return &MemoryRoot::instance.value();
}

void MemoryRoot::cleanup()
{
	MemoryRoot::instance.reset();
}

MemoryRoot::MemoryRoot()
{
	assert(!instance.has_value());
}

MemoryRoot::~MemoryRoot()
{
}

void MemoryRoot::visitHeaps(const std::function<void(MemoryHeap*)>& visitor)
{
	for (MemoryHeap* heap : livingHeaps) visitor(heap);
}

void MemoryRoot::updatePointers(const ObjectRelocator& remapper, std::set<void*>& visitRecord)
{
	for (MemoryHeap* heap : livingHeaps) heap->updatePointers(remapper, visitRecord);
	externals_updatePointers(remapper, visitRecord);
}

void MemoryRoot::externals_updatePointers(const ObjectRelocator& remapper, std::set<void*>& visitRecord)
{
	for (const auto& it : externalObjects)
	{
		const TypeName& ty = std::get<0>(it.second);
		ExternalObjectOptions opts = std::get<2>(it.second);
		remapper.transformObjectAddresses(
			it.first,
			ty,
			(uint8_t)opts&(uint8_t)ExternalObjectOptions::AllowFieldRecursion,
			((uint8_t)opts&(uint8_t)ExternalObjectOptions::AllowPtrRecursion) ? &visitRecord : nullptr
		);
	}
}

void MemoryRoot::registerHeap(MemoryHeap* heap)
{
	livingHeaps.emplace_back(heap);
}

void MemoryRoot::removeHeap(MemoryHeap* heap)
{
	auto it = std::find(livingHeaps.begin(), livingHeaps.end(), heap);
	assert(it != livingHeaps.end());
	livingHeaps.erase(it);
}

void MemoryRoot::registerExternal_impl(void* object, TypeName&& type, size_t size, ExternalObjectOptions options)
{
	typedef std::tuple<TypeName, size_t, ExternalObjectOptions> details_t;
	externalObjects.emplace(object, details_t(type, size, options));
}

void MemoryRoot::removeExternal(void* object)
{
	auto it = externalObjects.find(object);
	externalObjects.erase(it);
}

void MemoryRoot::visitExternals(const std::function<void(stix::SAnyRef, ExternalObjectOptions)>& visitor)
{
	for (const auto& kv : externalObjects)
	{
		stix::SAnyRef typedRef = stix::SAnyRef::makeUnsafe(kv.first, std::get<0>(kv.second));
		visitor(typedRef, std::get<2>(kv.second));
	}
}
