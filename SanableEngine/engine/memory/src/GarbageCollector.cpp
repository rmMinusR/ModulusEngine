#include "GarbageCollector.hpp"

#include "MemoryRoot.hpp"
#include "MemoryHeap.hpp"
#include "TypeInfo.hpp"

std::vector<stix::SAnyRef> GC::markReachable(stix::SAnyRef root, MemberVisibility visibilityMask)
{
	if (!root.has_value())
	{
		root = stix::SAnyRef::make(MemoryRoot::get());
	}

	std::vector<stix::SAnyRef> out{ root };

	for (size_t i = 0; i < out.size(); ++i)
	{
		// Cannot use range-based for: out will continue to expand
		
		// Search the frontier
		stix::SAnyRef frontier = out[i];
		const TypeInfo* ty = frontier.getType().resolve();
		if (ty)
		{
			ty->layout.walkFields(
				[&](const FieldInfo& fi)
				{
					if (fi.type.isDataPtr()) // FIXME @rsc handle upcast case
					{
						void** pptr = reinterpret_cast<void**>( fi.getValue(frontier.getUnsafe()) );
						stix::SAnyRef undiscovered = stix::SAnyRef::makeUnsafe(*pptr, fi.type);
						if (std::find(out.begin(), out.end(), undiscovered) == out.end())
						{
							out.push_back(undiscovered);
						}
					}
				},
				visibilityMask,
				true
			);
		}

	}

	return out;
}

std::vector<std::tuple<stix::SAnyRef, GenericTypedMemoryPool*>> GC::markUnreachable(ExternalObjectOptions requiredExtFlags)
{
	std::vector<stix::SAnyRef> reachable = markReachable();

	std::vector<std::tuple<stix::SAnyRef, GenericTypedMemoryPool*>> out;

	// Walk all externals
	MemoryRoot::get()->visitExternals(
		[&](stix::SAnyRef ext, ExternalObjectOptions options)
		{
			// Check external flags filter
			if ((int(options) & int(requiredExtFlags)) == int(requiredExtFlags))
			{
				// Attempt to add to frontier
				if (std::find(reachable.begin(), reachable.end(), ext) == reachable.end())
				{
					out.push_back({ ext, nullptr });
				}
			}
		}
	);

	// Walk all heaps
	MemoryRoot::get()->visitHeaps(
		[&](MemoryHeap* heap)
		{

			// Walk all pools
			heap->foreachPool(
				[&](GenericTypedMemoryPool* pool)
				{
					
					TypeName ty = pool->getContentsTypeName();

					// Walk all objects
					for (auto it = pool->cbegin(); it != pool->cend(); ++it)
					{
						// Mark as unreachable if not in reachable
						stix::SAnyRef undiscovered = stix::SAnyRef::makeUnsafe(*it, ty);
						if (std::find(reachable.begin(), reachable.end(), undiscovered) == reachable.end())
						{
							out.push_back({ undiscovered, pool });
						}
					}

				}
			);

		}
	);

	return out;
}

size_t GC::markAndSweep()
{
	std::vector<std::tuple<stix::SAnyRef, GenericTypedMemoryPool*>> deletable = markUnreachable(ExternalObjectOptions::AllowDelete);
	size_t num_deleted = 0;
	for (const auto& i : deletable)
	{
		stix::SAnyRef obj = std::get<0>(i);
		GenericTypedMemoryPool* owningPool = std::get<1>(i);

		const TypeInfo* ty = obj.getType().resolve();
		if (ty && ty->isLoaded())
		{
			if (ty->capabilities.richDtor.has_value())
			{
				const auto& dtor = ty->capabilities.richDtor.value();
				if (dtor.thunk.has_value())
				{
					// We can delete it!
					++num_deleted;

					if (owningPool)
					{
						owningPool->release(obj.getUnsafe());
					}
					else
					{
						dtor.thunk.value().invoke(stix::SAnyRef(), obj, {});
					}
				}
			}
		}
	}
	return num_deleted;
}
