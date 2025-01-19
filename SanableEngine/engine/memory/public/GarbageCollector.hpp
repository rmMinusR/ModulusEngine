#pragma once

#include <tuple>

#include <MemberInfo.hpp>
#include <SAny.hpp>

#include "ExternalObject.hpp"

class GenericTypedMemoryPool;

namespace GC
{
	/// <summary>
	/// Get a list of all objects reachable from this one
	/// </summary>
	/// <param name="root">If no root is provided, root will be MemoryRoot</param>
	/// <param name="visibilityMask">Which members to traverse</param>
	std::vector<stix::SAnyRef> markReachable(stix::SAnyRef root = {}, MemberVisibility visibilityMask = MemberVisibility::All);

	/// <summary>
	/// List all objects considered unreachable from MemoryRoot
	/// </summary>
	std::vector<std::tuple<stix::SAnyRef, GenericTypedMemoryPool*>> markUnreachable(ExternalObjectOptions requiredExtFlags = ExternalObjectOptions::None);

	/// <summary>
	/// Deletes all unreachable objects
	/// </summary>
	/// <returns>Number of destroyed objects</returns>
	size_t markAndSweep();
}
