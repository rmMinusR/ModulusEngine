#include "RawMemoryPool.hpp"

#include <cstdlib>
#include <cassert>
#include <iostream>

#include "MemoryPoolCommon.hpp"

using namespace std;

void* RawMemoryPool::idToPtr(id_t id) const
{
	return ((char*)getObjectDataBlock()) + mObjectSize * id;
}

RawMemoryPool::id_t RawMemoryPool::ptrToId(void* ptr) const
{
	assert(contains(ptr));
	ptrdiff_t offset = ((char*)ptr) - ((char*)getObjectDataBlock());
	assert((offset % mObjectSize) == 0);
	return offset / mObjectSize;
}

bool RawMemoryPool::isAliveById(id_t id) const
{
	uint8_t* block = getLivingListBlock() + (id / 8);
	uint8_t bitmask = 1 << (id % 8);
	return *block & bitmask;
}

void RawMemoryPool::debugWarnUnreleased() const
{
	printf("WARNING: A release hook was set, but objects (%s) weren't properly released\n", debugName.c_str());
}

void RawMemoryPool::debugWarnUnreleased(void* obj) const
{
	printf(" -> %p\n", obj);
}

RawMemoryPool::RawMemoryPool() :
	initHook(nullptr),
	releaseHook(nullptr),
	mMemoryBlock(nullptr),
	mMaxNumObjects(0),
	mNumAllocatedObjects(0),
	mObjectSize(0)
{
}

RawMemoryPool::RawMemoryPool(size_t maxNumObjects, size_t objectSize) : RawMemoryPool()
{
	//Disabled: Let's just trust the caller. TypedMemoryPool has safety anyway.
	//make objectSize a power of 2 - used for padding
	//assert(isPowerOfTwo(objectSize));
	//objectSize = getClosestPowerOf2LargerThan(objectSize);
	//if (objectSize < 4) objectSize = 4;

	//allocate the memory
	mMemoryBlock = malloc(getLivingListSpaceRequired() + objectSize * maxNumObjects);

	//set member variables
	mMaxNumObjects = maxNumObjects;
	mNumAllocatedObjects = 0;
	mObjectSize = objectSize;

	//Mark all as unused
	memset(getLivingListBlock(), 0x00, getLivingListSpaceRequired());
}

RawMemoryPool::~RawMemoryPool()
{
	//Call release hook on living objects
	reset();

	::free(mMemoryBlock);
	mMemoryBlock = nullptr;
}

RawMemoryPool::RawMemoryPool(RawMemoryPool&& rhs) :
	RawMemoryPool(rhs.mMaxNumObjects, rhs.mObjectSize)
{
	std::swap(this->mMemoryBlock, rhs.mMemoryBlock);
}

void RawMemoryPool::reset()
{
	//Call release hook on living objects
	if (releaseHook && mNumAllocatedObjects > 0)
	{
		debugWarnUnreleased();

		for (size_t i = 0; i < mMaxNumObjects; i++)
		{
			void* obj = idToPtr(i);
			if (isAlive(obj))
			{
				debugWarnUnreleased(obj);
				releaseHook(obj);
			}
		}
	}
	
	//Mark all as unused
	memset(getLivingListBlock(), 0x00, getLivingListSpaceRequired());

	//reset count of allocated objects
	mNumAllocatedObjects = 0;
}

void RawMemoryPool::resizeObjects(size_t newSize, MemoryMapper* mapper)
{
	newSize = getClosestPowerOf2LargerThan(newSize);
	if (newSize != mObjectSize)
	{
		//Allocate new backing block
		void* newMemoryBlock = malloc(getLivingListSpaceRequired() + newSize*mMaxNumObjects);
		void* newObjectBlock = ((char*)newMemoryBlock) + getLivingListSpaceRequired();

		//Transfer living list
		void* newLivingBlock = newMemoryBlock;
		memcpy_s(newLivingBlock, getLivingListSpaceRequired(), getLivingListBlock(), getLivingListSpaceRequired());

		//Don't bother shuffling members. Assume the caller knows what they're doing.

		if (newSize > mObjectSize)
		{
			//Size increased. Move starting from highest address.
			for (size_t i = mMaxNumObjects-1; i != (size_t)-1; i--)
			{
				void* src = ((uint8_t*) getObjectDataBlock()) + (i * mObjectSize);
				void* dst = ((uint8_t*)     newObjectBlock  ) + (i * newSize);
				mapper->rawMove(dst, src, std::min(mObjectSize, newSize));
			}
		}
		else if (newSize < mObjectSize)
		{
			//Size decreased. Move starting from lowest address.
			for (size_t i = 0; i < mMaxNumObjects; i++)
			{
				void* src = ((uint8_t*) getObjectDataBlock()) + (i * mObjectSize);
				void* dst = ((uint8_t*)     newObjectBlock  ) + (i * newSize);
				mapper->rawMove(dst, src, std::min(mObjectSize, newSize));
			}
		}
		
		::free(mMemoryBlock);
		mMemoryBlock = newMemoryBlock;
		mObjectSize = newSize;
	}
}

void* RawMemoryPool::allocate()
{
	if (mNumAllocatedObjects >= mMaxNumObjects)
	{
		return NULL;
	}

	if (mNumAllocatedObjects < mMaxNumObjects)
	{
		mNumAllocatedObjects++;

		//Scan for first free address
		int id = 0;
		void* ptr;
		for (ptr = idToPtr(id); contains(ptr); ptr = idToPtr(++id))
		{
			if (!isAlive(ptr)) break;
		}

		setFree(id, false);
		if (initHook) initHook(ptr);
		return ptr;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void RawMemoryPool::release(void* ptr)
{
	//make sure that the address passed in is actually one managed by this pool
	if (contains(ptr))
	{
		if (releaseHook) releaseHook(ptr);

		//add address back to free list
		setFree(ptrToId(ptr), true);

		mNumAllocatedObjects--;
	}
	else
	{
		cerr << "ERROR: object freed from a pool that doesn't manage it\n";
		assert(false);
	}
}

bool RawMemoryPool::contains(void* ptr) const
{
	return idToPtr(0) <= ptr && ptr < idToPtr(mMaxNumObjects);
}

void RawMemoryPool::setFree(id_t id, bool isFree)
{
	uint8_t* chunk = getLivingListBlock() + (id / 8);
	uint8_t bitmask = 1 << (id%8);
	if (isFree) *chunk &= ~bitmask;
	else        *chunk |=  bitmask;
}

RawMemoryPool::const_iterator::const_iterator(RawMemoryPool const* pool, id_t index) :
	pool(pool),
	index(index)
{
}

void* RawMemoryPool::const_iterator::operator*() const
{
	return pool->idToPtr(index);
}

RawMemoryPool::const_iterator RawMemoryPool::const_iterator::operator++()
{
	//Advance one
	index++;

	//Skip any IDs that aren't alive
	while (index < pool->mMaxNumObjects && !pool->isAliveById(index)) index++;

	return *this;
}

RawMemoryPool::const_iterator RawMemoryPool::cbegin() const
{
	return const_iterator(this, 0);
}

RawMemoryPool::const_iterator RawMemoryPool::cend() const
{
	return const_iterator(this, mMaxNumObjects);
}