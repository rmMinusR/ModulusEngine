#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>

#include "ModuleTypeRegistry.hpp"

/// <summary>
/// Reflection(ish) for all types currently in runtime context
/// </summary>
class GlobalTypeRegistry
{
public:
	typedef std::wstring_view module_key_t;

	class Snapshot
	{
	public:
		STIX_API Snapshot();
		STIX_API ~Snapshot();

		STIX_API Snapshot(const Snapshot& cpy);
		STIX_API Snapshot(Snapshot&& mov);
		STIX_API Snapshot& operator=(const Snapshot& cpy);
		STIX_API Snapshot& operator=(Snapshot&& mov);
	private:
		std::unordered_map<TypeName, size_t> hashes;
		size_t overallHash;
		friend class GlobalTypeRegistry;
		friend struct std::hash<GlobalTypeRegistry::Snapshot>;
	};

private:
	static std::unordered_map<module_key_t, ModuleTypeRegistry> modules;

	STIX_API static void _makeSnapshot_internal();
	static std::optional<Snapshot> _cachedSnapshotValue;
	
public:
	/// <summary>
	/// Look up a type's metadata, if it is currently alive. If not, returns nullptr.
	/// </summary>
	STIX_API static TypeInfo const* lookupType(const TypeName& name);

	STIX_API static ModuleTypeRegistry const* getModule(const module_key_t& key);

	//////////// INTERNAL FUNCTIONS ////////////

	/// <summary>
	/// Register a new module. If one already exists with the same name, it will be unloaded first.
	/// </summary>
	STIX_API static void loadModule(std::string key, const ModuleTypeRegistry& newTypes);
	STIX_API static void loadModule(module_key_t key, const ModuleTypeRegistry& newTypes);

	STIX_API static void loadIntrinsics();
	STIX_API static std::wstring_view INTRINSICS_MODULE_ID;

	/// <summary>
	/// Mark a module as unloaded
	/// </summary>
	STIX_API static void unloadModule(std::string key);
	STIX_API static void unloadModule(module_key_t key);

	/// <summary>
	/// Attempt to find the exact type of a void pointer
	/// </summary>
	/// <param name="obj">Object of unknown type</param>
	/// <param name="size">Size of object. Can be semi-inaccurate as long as we can safely read memory in the specified range.</param>
	/// <param name="hint">Parent type, can be null if unknown</param>
	/// <returns>Pointer to matching type, or null if none was found</returns>
	STIX_API static TypeInfo const* snipeType(void* obj, size_t size, TypeInfo const* hint = nullptr);

	/// <summary>
	/// Get the names of any types that have been altered via updateModule since last call.
	/// </summary>
	[[nodiscard]] STIX_API static std::unordered_set<TypeName> getDirtyTypes(const Snapshot& prev);

	STIX_API static const Snapshot& makeSnapshot();

	/// <summary>
	/// Completely wipe internal state. Useful for shutdown cleanup and unit testing.
	/// </summary>
	STIX_API static void clear();
};


template<> struct std::hash<GlobalTypeRegistry::Snapshot>
{
	size_t operator()(const GlobalTypeRegistry::Snapshot& v) { return v.overallHash; }
};
