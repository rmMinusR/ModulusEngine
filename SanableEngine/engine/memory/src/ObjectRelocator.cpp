#include "ObjectRelocator.hpp"

#include <cstring>
#include <cassert>

#include "TypeInfo.hpp"

void ObjectRelocator::rawMove(void* dst, void* src, size_t bytesToMove)
{
	memcpy(dst, src, bytesToMove); //Do actual move op
	if (USE_INVALID_DATA_FILL) memset(src, INVALID_DATA_FILL_VALUE, bytesToMove); //Fill old memory

	//Allow calls on nullptr -- only log if not null.
	if (this) logMove(dst, src, bytesToMove);
}

void ObjectRelocator::logMove(void* dst, void* src, size_t bytesToMove)
{
	if (!this) return;

	RelocOp op;
	op.src = src;
	op.dst = dst;
	op.blockSize = bytesToMove;
	opLog.push_back(op);
}

void* ObjectRelocator::transformAddress(void* ptr, size_t ptrSize) const
{
	for (const RelocOp& op : opLog)
	{
		void* srcEnd = ((char*)op.src) + op.blockSize; //First byte NOT in remapped block
		if (op.src <= ptr && ptr < srcEnd)
		{
			void* lastByte = ((char*)ptr) + ptrSize - 1;
			assert(lastByte < srcEnd); //Ensure last byte is also in range. No object shearing!
			ptr = ((char*)ptr) - ((char*)op.src) + ((char*)op.dst);
		}
	}

	return ptr;
}

void ObjectRelocator::transformComposite(void* object, const TypeInfo* type, bool recurseFields, std::set<void*>* recursePointers) const
{
	assert(type->isComposite());
	type->layout.walkFields([&](const FieldInfo& fi)
	{
		std::optional<TypeName> pointee = type->name.dereference();
		if (pointee.has_value())
		{
			void** pPtr = (void**)object;
			void* srcPtr = *pPtr;
			*pPtr = transformAddress(srcPtr, sizeof(void*));
			
			if (recursePointers && *pPtr && recursePointers->count(srcPtr) == 0)
			{
				recursePointers->emplace(srcPtr);
				//TODO polymorphism check, snipe and downcast
				transformObjectAddresses(srcPtr, pointee.value(), recurseFields, recursePointers);
			}
		}
		else if (recurseFields) transformObjectAddresses(fi.getValue(object), fi.type, recurseFields, recursePointers);
	});
}

void ObjectRelocator::transformObjectAddresses(void* object, const TypeName& typeName, bool recurseFields, std::set<void*>* recursePointers) const
{
	std::optional<TypeName> pointee = typeName.dereference();
	if (pointee.has_value())
	{
		void** pPtr = (void**)object;
		void* srcPtr = *pPtr;
		void* dstPtr = *pPtr = transformAddress(srcPtr, 1); //FIXME this wants size of pointed-to type for safety

		if (recursePointers && *pPtr && recursePointers->count(srcPtr) == 0 && recursePointers->count(dstPtr) == 0)
		{
			recursePointers->emplace(srcPtr);
			recursePointers->emplace(dstPtr);
			//TODO polymorphism check, snipe and downcast
			transformObjectAddresses(dstPtr, pointee.value(), recurseFields, recursePointers);
		}
	}
	else if (recurseFields && typeName.isComposite())
	{
		if (recursePointers) recursePointers->emplace(object);
		const TypeInfo* type = typeName.resolve();
		if (type) transformComposite(object, type, recurseFields, recursePointers);
	}
	else
	{
		if (recursePointers) recursePointers->emplace(object);
	}
}

void ObjectRelocator::clear()
{
	opLog.clear();
}