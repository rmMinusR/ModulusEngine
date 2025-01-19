#pragma once

#include <optional>
#include <map>
#include <functional>

#include "dllapi.h"

class MemoryHeap;

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
	ENGINEMEM_API void ensureFresh();
	
private:
	static std::optional<MemoryRoot> instance;

	std::vector<MemoryHeap*> livingHeaps;
	friend class MemoryHeap;
	void registerHeap(MemoryHeap* heap);
	void removeHeap(MemoryHeap* heap);
};
