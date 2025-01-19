#pragma once

#include "dllapi.h"

#include <cassert>
#include <vector>
#include <algorithm>
#include <string>

#include "MemoryMapper.hpp"

//Void pointers of a given max size (not recommended)
class RawMemoryPool
{
protected:
	typedef uint16_t id_t;

	ENGINEMEM_API void* idToPtr(id_t id) const;
	ENGINEMEM_API id_t ptrToId(void* ptr) const;
	ENGINEMEM_API bool isAliveById(id_t id) const;
	ENGINEMEM_API void setAlive(id_t id, bool isAlive);

	std::string debugName;
	ENGINEMEM_API void debugWarnUnreleased() const;
	ENGINEMEM_API void debugWarnUnreleased(void* obj) const;

private:
	RawMemoryPool();
public:
	ENGINEMEM_API RawMemoryPool(size_t maxNumObjects, size_t objectSize, size_t objectAlign);
	ENGINEMEM_API virtual ~RawMemoryPool();

	//Idiotproofing against myself
	RawMemoryPool(const RawMemoryPool&) = delete;
	RawMemoryPool(RawMemoryPool&&);

	//Resizes individual allocations, moving them as appropriate.
	//This will break any existing pointers, unless a mapper is used to fix them.
	ENGINEMEM_API void resizeObjects(size_t newSize, size_t newAlign, MemoryMapper* mapper = nullptr);

	//Changes the maximum object count.
	//This will break any existing pointers, unless a mapper is used to fix them.
	ENGINEMEM_API void setMaxNumObjects(size_t newCount, MemoryMapper* mapper = nullptr);

	typedef void (*hook_t)(void*);

	//Allocates raw memory. Returns null if out of memory.
	//Set hook if type requires special initialization
	[[nodiscard]] ENGINEMEM_API void* allocate();
	hook_t initHook;

	//Deallocates raw memory.
	//Set hook if type requires special cleanup
	ENGINEMEM_API void release(void* obj);
	hook_t releaseHook;

	ENGINEMEM_API bool contains(void* ptr) const;
	inline bool isAlive(void* ptr) const
	{
		return isAliveById(ptrToId(ptr)); //Convert to id and defer to main impl
	}
	ENGINEMEM_API void reset();//doesn't reallocate memory but does reset free list and num allocated objects

	inline size_t getMaxObjectSize()  const { return mObjectSize; }
	inline size_t getMaxNumObjects() const { return mMaxNumObjects; }
	inline size_t getNumFreeObjects() const { return mMaxNumObjects - mNumAllocatedObjects; }
	inline size_t getNumAllocatedObjects() const { return mNumAllocatedObjects; }

protected:
	uint8_t* mLivingListBlock; //Free list is a dynamically-sized bitset. 1 = alive, 0 = free.
	void* mDataBlock; //Storage for allocated objects
	ENGINEMEM_API uint8_t* getLivingListBlock() const { return mLivingListBlock; }
	ENGINEMEM_API void* getObjectDataBlock() const { return mDataBlock; }

	size_t mMaxNumObjects;
	size_t mNumAllocatedObjects;
	size_t mObjectSize;
	size_t mObjectAlign;

public:
	class const_iterator
	{
		RawMemoryPool const* pool;
		id_t index;

		const_iterator(RawMemoryPool const* pool, id_t index);

		friend class RawMemoryPool;

	public:
		ENGINEMEM_API void* operator*() const;

		ENGINEMEM_API const_iterator operator++();
		ENGINEMEM_API const_iterator operator+=(size_t offset);
		ENGINEMEM_API const_iterator operator+(size_t offset) const;

		ENGINEMEM_API inline bool operator!=(const const_iterator& other) const { return pool == other.pool && index != other.index; }
		ENGINEMEM_API inline bool operator==(const const_iterator& other) const { return pool == other.pool && index == other.index; }
	};
	ENGINEMEM_API const_iterator cbegin() const;
	ENGINEMEM_API const_iterator cend() const;
};
