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

void MemoryRoot::ensureFresh()
{
	for (MemoryHeap* heap : livingHeaps) heap->ensureFresh();
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
