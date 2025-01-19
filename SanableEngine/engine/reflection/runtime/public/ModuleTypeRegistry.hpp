#pragma once

#include <vector>

#include "TypeInfo.hpp"

class GlobalTypeRegistry;

/// <summary>
/// Reflection(ish) for all types within a module (read: DLL/plugin).
/// </summary>
class ModuleTypeRegistry
{
public:
	STIX_API TypeInfo const* lookupType(const TypeName& name) const;
	STIX_API const std::vector<TypeInfo>& getTypes() const;

	/// <summary>
	/// Attempt to find the exact type of a void pointer
	/// </summary>
	/// <param name="obj">Object of unknown type</param>
	/// <param name="size">Size of object. Can be semi-inaccurate as long as we can safely read memory in the specified range.</param>
	/// <param name="hint">Parent type, can be null if unknown</param>
	/// <returns>Pointer to matching type, or null if none was found</returns>
	STIX_API TypeInfo const* snipeType(void* obj, size_t size, TypeInfo const* hint = nullptr) const;

	STIX_API ModuleTypeRegistry();
	STIX_API ~ModuleTypeRegistry();

	STIX_API ModuleTypeRegistry(const ModuleTypeRegistry& cpy);
	STIX_API ModuleTypeRegistry(ModuleTypeRegistry&& mov);
	STIX_API ModuleTypeRegistry& operator=(const ModuleTypeRegistry& cpy);
	STIX_API ModuleTypeRegistry& operator=(ModuleTypeRegistry&& mov);
	
private:
	std::vector<TypeInfo> types;
	friend class TypeBuilder;

	void doLateBinding();
	friend class GlobalTypeRegistry;
};
