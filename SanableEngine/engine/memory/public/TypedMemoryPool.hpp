#pragma once

#include "RawMemoryPool.hpp"
#include "TypeInfo.hpp"
#include "GlobalTypeRegistry.hpp"

class MemoryHeap;
class GenericTypedMemoryPool;

//Strongly typed pointers (recommended)
//Just a compile-time typesafe view onto GenericTypedMemoryPool, that's also reload-safe
template<typename TObj>
class TypedMemoryPool
{
	static_assert(!std::is_abstract_v<TObj>);

private:
	GenericTypedMemoryPool* impl;
public:
	TypedMemoryPool(GenericTypedMemoryPool* impl) : impl(impl) {}

	//Allocates memory and creates an object.
	template<typename... TCtorArgs>
	TObj* emplace(TCtorArgs... ctorArgs)
	{
		//Allocate memory
		TObj* pObj = (TObj*) impl->allocate();
		
		//Construct object
		if (pObj) new (pObj) TObj(ctorArgs...);

		return pObj;
	}

	//Pass through
	inline void release(TObj* obj) { impl->release(obj); }

	inline GenericTypedMemoryPool* asGeneric() { return impl; }
	inline GenericTypedMemoryPool const* asGeneric() const { return impl; }

	//Pass through with type safety
	class const_iterator
	{
	private:
		RawMemoryPool::const_iterator inner;
		inline const_iterator(RawMemoryPool::const_iterator inner) : inner(inner) {}
		friend class TypedMemoryPool<TObj>;

	public:
		inline TObj& operator*() const { return *reinterpret_cast<TObj*>(*inner); }
		inline TObj* operator->() const { return reinterpret_cast<TObj*>(*inner); }

		inline const_iterator operator++()              { ++inner;         return *this; }
		inline const_iterator operator+=(size_t offset) { inner += offset; return *this; }
		inline const_iterator operator+(size_t offset) const { return const_iterator(*this) += offset; }

		inline bool operator!=(const const_iterator& other) const { return inner != other.inner; }
		inline bool operator==(const const_iterator& other) const { return inner == other.inner; }
	};

	inline const_iterator cbegin() const { return const_iterator(impl->cbegin()); }
	inline const_iterator cend  () const { return const_iterator(impl->cend  ()); }

protected:
	TypedMemoryPool(TypedMemoryPool&&) = delete;
	TypedMemoryPool(const TypedMemoryPool&) = delete;
};


//Backend for TypedMemoryPool so we can still safely access common data
class GenericTypedMemoryPool : public RawMemoryPool
{
protected:
	TypeInfo contentsType;
	TypedMemoryPool<void> view; //Needs to be cast to be used safely

	ENGINEMEM_API GenericTypedMemoryPool(size_t maxNumObjects, const TypeInfo& contentsType);
public:
	ENGINEMEM_API ~GenericTypedMemoryPool();

	ENGINEMEM_API bool isLoaded() const;
	ENGINEMEM_API const TypeInfo* getContentsType() const;
	ENGINEMEM_API TypeName getContentsTypeName() const;

	template<typename TObj>
	[[nodiscard]] static GenericTypedMemoryPool* create(size_t maxNumObjects = 64)
	{
		const TypeInfo* existing = GlobalTypeRegistry::lookupType(TypeName::create<TObj>());
		return new GenericTypedMemoryPool(
			maxNumObjects,
			existing ? *existing : TypeInfo::createDummy<TObj>() //No need to resolve dummy TypeInfo here. Engine will call refreshObjects after all TypeInfos are registered.
		);
	}

	template<typename TObj>
	inline TypedMemoryPool<TObj>* getView()
	{
		assert(TypeName::create<TObj>() == contentsType.name);
		return reinterpret_cast<TypedMemoryPool<TObj>*>(&view);
	}

	//INTERNAL USE ONLY
	ENGINEMEM_API void refreshObjects(const TypeInfo& newTypeData, MemoryMapper* remapper);
};
