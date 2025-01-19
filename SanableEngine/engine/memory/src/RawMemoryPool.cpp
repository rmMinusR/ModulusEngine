#include "RawMemoryPool.hpp"

#include <cstdlib>
#include <cassert>
#include <iostream>

#include "alloc_detail.h"

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

void RawMemoryPool::setAlive(id_t id, bool isAlive)
{
	uint8_t* chunk = getLivingListBlock() + (id / 8);
	uint8_t bitmask = 1 << (id % 8);
	if (isAlive) *chunk |= bitmask;
	else		 *chunk &= ~bitmask;
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
	mLivingListBlock(nullptr),
	mDataBlock(nullptr),
	mMaxNumObjects(0),
	mNumAllocatedObjects(0),
	mObjectSize(0)
{
}

RawMemoryPool::RawMemoryPool(size_t maxNumObjects, size_t objectSize, size_t objectAlign) : RawMemoryPool()
{
	//Set trivial fields
	mMaxNumObjects = maxNumObjects;
	mNumAllocatedObjects = 0;
	mObjectSize = objectSize;
	mObjectAlign = objectAlign;

	//Allocate memory blocks
	mDataBlock = ALIGNED_ALLOC(mObjectSize * mMaxNumObjects, mObjectAlign);

	size_t livingListSpaceRequired = ceil(mMaxNumObjects / 8.0f);
	mLivingListBlock = (uint8_t*) malloc(livingListSpaceRequired);
	memset(getLivingListBlock(), 0x00, livingListSpaceRequired); //Mark all as unused
}

RawMemoryPool::~RawMemoryPool()
{
	//Call release hook on living objects
	reset();

	free(mLivingListBlock);
	mLivingListBlock = nullptr;

	ALIGNED_FREE(mDataBlock);
	mDataBlock = nullptr;
}

RawMemoryPool::RawMemoryPool(RawMemoryPool&& mov) :
	RawMemoryPool(mov.mMaxNumObjects, mov.mObjectSize, mov.mObjectAlign)
{
	this->mLivingListBlock = mov.mLivingListBlock;
	this->mDataBlock       = mov.mDataBlock      ;

	mov.mLivingListBlock = nullptr;
	mov.mDataBlock       = nullptr;
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
	memset(getLivingListBlock(), 0x00, ceil(mMaxNumObjects / 8.0f));

	//reset count of allocated objects
	mNumAllocatedObjects = 0;
}

void RawMemoryPool::resizeObjects(size_t newSize, size_t newAlign, ObjectRelocator* mapper)
{
	if (newSize != mObjectSize || newAlign != mObjectAlign)
	{
		//Allocate new backing block
		void* newDataBlock = ALIGNED_ALLOC(newSize*mMaxNumObjects, newAlign);
		
		//Don't bother shuffling members. Assume the caller knows what they're doing.

		if (newSize > mObjectSize)
		{
			//Size increased. Move starting from highest address.
			for (size_t i = mMaxNumObjects-1; i != (size_t)-1; i--)
			{
				void* src = ((uint8_t*) getObjectDataBlock()) + (i * mObjectSize);
				void* dst = ((uint8_t*)     newDataBlock    ) + (i * newSize);
				mapper->rawMove(dst, src, std::min(mObjectSize, newSize));
			}
		}
		else if (newSize < mObjectSize)
		{
			//Size decreased. Move starting from lowest address.
			for (size_t i = 0; i < mMaxNumObjects; i++)
			{
				void* src = ((uint8_t*) getObjectDataBlock()) + (i * mObjectSize);
				void* dst = ((uint8_t*)     newDataBlock    ) + (i * newSize);
				mapper->rawMove(dst, src, std::min(mObjectSize, newSize));
			}
		}
		
		ALIGNED_FREE(mDataBlock);
		mDataBlock = newDataBlock;
		mObjectSize = newSize;
		mObjectAlign = newAlign;
	}
}

void RawMemoryPool::setMaxNumObjects(size_t newCount, ObjectRelocator* mapper)
{
	if (newCount != mMaxNumObjects)
	{
		//Allocate new backing block
		void* newDataBlock = ALIGNED_ALLOC(mObjectSize*newCount, mObjectAlign);
		
		mapper->rawMove(newDataBlock, mDataBlock, mObjectSize*std::min(mMaxNumObjects, newCount));
		
		ALIGNED_FREE(mDataBlock);
		mDataBlock = newDataBlock;
	}
}

void* RawMemoryPool::allocate()
{
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

		setAlive(id, true);
		if (initHook) initHook(ptr);
		return ptr;
	}
	else
	{
		return nullptr;
	}
}

void RawMemoryPool::release(void* ptr)
{
	//make sure that the address passed in is actually one managed by this pool
	if (!contains(ptr))
	{
		cerr << "ERROR: object freed from a pool (" << debugName << ") that doesn't manage it\n";
		return;
	}
	
	//Make sure the specified object is alive
	if (!isAlive(ptr))
	{
		cerr << "ERROR: object freed (" << debugName << "), but was already dead\n";
		return;
	}

	//Main business logic
	if (releaseHook) releaseHook(ptr);
	setAlive(ptrToId(ptr), false);
	mNumAllocatedObjects--;
}

bool RawMemoryPool::contains(void* ptr) const
{
	return idToPtr(0) <= ptr && ptr < idToPtr(mMaxNumObjects);
}

RawMemoryPool::const_iterator::const_iterator(RawMemoryPool const* pool, id_t index) :
	pool(pool),
	index(index)
{
	//Skip any IDs that aren't alive
	while (this->index < pool->mMaxNumObjects && !pool->isAliveById(this->index)) this->index++;
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