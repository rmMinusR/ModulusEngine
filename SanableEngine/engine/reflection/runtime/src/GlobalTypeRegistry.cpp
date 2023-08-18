#include "GlobalTypeRegistry.hpp"

#include <unordered_set>
#include <cassert>

std::unordered_map<GlobalTypeRegistry::module_key_t, ModuleTypeRegistry> GlobalTypeRegistry::modules;
std::unordered_set<TypeName> GlobalTypeRegistry::dirtyTypes;

TypeInfo const* GlobalTypeRegistry::lookupType(const TypeName& name)
{
	for (const auto& i : modules)
	{
		TypeInfo const* out = i.second.lookupType(name);
		if (out) return out;
	}
	return nullptr;
}

void GlobalTypeRegistry::loadModule(module_key_t key, const ModuleTypeRegistry& newTypes)
{
	//If a module already exists with the same name, unload first
	if (modules.find(key) != modules.cend()) unloadModule(key);

	//Register types
	auto it = modules.emplace(key, newTypes).first;

	//Mark all known names as dirty
	for (const TypeInfo& i : newTypes.getTypes()) dirtyTypes.emplace(i.name);
}

void GlobalTypeRegistry::unloadModule(module_key_t key)
{
	auto it = modules.find(key);
	assert(it != modules.cend());

	//Mark all known names as dirty
	const ModuleTypeRegistry& oldTypes = it->second;
	for (const TypeInfo& i : oldTypes.getTypes()) dirtyTypes.emplace(i.name);

	//Unregister types
	modules.erase(it);
}

std::unordered_set<TypeName> GlobalTypeRegistry::getDirtyTypes()
{
	std::unordered_set<TypeName> out;
	std::swap(out, dirtyTypes);
	return out;
}

void GlobalTypeRegistry::clear()
{
	modules.clear();
	dirtyTypes.clear();
}