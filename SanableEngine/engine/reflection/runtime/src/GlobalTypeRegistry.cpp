#include "GlobalTypeRegistry.hpp"

#include <unordered_set>
#include <cassert>
#include <stdlib.h>

#include "TypeBuilder.hpp"

std::unordered_map<GlobalTypeRegistry::module_key_t, ModuleTypeRegistry> GlobalTypeRegistry::modules;
std::optional<GlobalTypeRegistry::Snapshot> GlobalTypeRegistry::_cachedSnapshotValue;

TypeInfo const* GlobalTypeRegistry::lookupType(const TypeName& name)
{
	for (const auto& i : modules)
	{
		TypeInfo const* out = i.second.lookupType(name);
		if (out) return out;
	}
	return nullptr;
}

ModuleTypeRegistry const* GlobalTypeRegistry::getModule(const module_key_t& key)
{
	auto it = modules.find(key);
	return it != modules.end() ? &it->second : nullptr;
}

void GlobalTypeRegistry::loadModule(std::string key, const ModuleTypeRegistry& newTypes)
{
	std::wstring wide(key.length(), ' ');
	std::mbstowcs(wide.data(), key.data(), key.length());
	loadModule(wide, newTypes);
}

void GlobalTypeRegistry::loadModule(module_key_t key, const ModuleTypeRegistry& newTypes)
{
	//If a module already exists with the same name, unload first
	if (modules.find(key) != modules.cend()) unloadModule(key);

	GlobalTypeRegistry::_cachedSnapshotValue = std::nullopt;

	//Register types
	auto it = modules.emplace(key, newTypes).first;
	
	//Finalize late-binding info
	it->second.doLateBinding();
}

std::wstring_view GlobalTypeRegistry::INTRINSICS_MODULE_ID = L"<C++ intrinsics>";

void GlobalTypeRegistry::loadIntrinsics()
{
	assert(getModule(INTRINSICS_MODULE_ID) == nullptr);

	ModuleTypeRegistry intrinsics;

#define DECL_INTRINSIC(ty) TypeBuilder::create<ty>().registerType(&intrinsics)

	// Character types

	DECL_INTRINSIC(char);
	DECL_INTRINSIC(wchar_t);

	// Integral types

	DECL_INTRINSIC(  signed char);
	DECL_INTRINSIC(unsigned char);

	DECL_INTRINSIC(   short int);
	DECL_INTRINSIC(unsigned short int);

	DECL_INTRINSIC(         int);
	DECL_INTRINSIC(unsigned int);
	
	DECL_INTRINSIC(         long int);
	DECL_INTRINSIC(unsigned long int);

	DECL_INTRINSIC(         long long int);
	DECL_INTRINSIC(unsigned long long int);

	// Decimal types

	DECL_INTRINSIC(float);
	DECL_INTRINSIC(double);
	DECL_INTRINSIC(long double);

	// Misc

	DECL_INTRINSIC(bool);
	DECL_INTRINSIC(decltype(nullptr));

#undef DECL_INTRINSIC

	loadModule(INTRINSICS_MODULE_ID, intrinsics);
}

TypeInfo const* GlobalTypeRegistry::snipeType(void* obj, size_t size, TypeInfo const* hint)
{
	for (const auto& kv : modules)
	{
		TypeInfo const* out = kv.second.snipeType(obj, size, hint);
		if (out) return out;
	}
	return nullptr;
}

void GlobalTypeRegistry::unloadModule(std::string key)
{
	std::wstring wide(key.length(), ' ');
	std::mbstowcs(wide.data(), key.data(), key.length());
	unloadModule(wide);
}

void GlobalTypeRegistry::unloadModule(module_key_t key)
{
	auto it = modules.find(key);
	assert(it != modules.cend());

	//Unregister types
	modules.erase(it);

	GlobalTypeRegistry::_cachedSnapshotValue = std::nullopt;
}

std::unordered_set<TypeName> GlobalTypeRegistry::getDirtyTypes(const Snapshot& prev)
{
	//End fast if nothing changed
	if (makeSnapshot().overallHash == prev.overallHash) return std::unordered_set<TypeName>(0);

	std::unordered_set<TypeName> out;
	for (const auto& i : prev.hashes) if (!GlobalTypeRegistry::lookupType(i.first)) out.emplace(i.first); //Has been unloaded since snapshot
	for (const auto& m : modules)
	{
		for (const TypeInfo& t : m.second.types)
		{
			if (!prev.hashes.count(t.name)) out.emplace(t.name); //Has been loaded since snapshot
			else if (std::hash<TypeInfo>{}(t) != prev.hashes.at(t.name)) out.emplace(t.name); //Content changed
		}
	}
	return out;
}

void GlobalTypeRegistry::_makeSnapshot_internal()
{
	_cachedSnapshotValue = std::nullopt;
	_cachedSnapshotValue.emplace();
	_cachedSnapshotValue.value().overallHash = 5381;
	for (const auto& m : modules)
	{
		for (const TypeInfo& t : m.second.types)
		{
			size_t tHash = std::hash<TypeInfo>{}(t);
			_cachedSnapshotValue.value().hashes.emplace(t.name, tHash);
			_cachedSnapshotValue.value().overallHash *= 33;
			_cachedSnapshotValue.value().overallHash += tHash;
		}
	}
}

const GlobalTypeRegistry::Snapshot& GlobalTypeRegistry::makeSnapshot()
{
	if (!GlobalTypeRegistry::_cachedSnapshotValue.has_value()) _makeSnapshot_internal();
	return GlobalTypeRegistry::_cachedSnapshotValue.value();
}

GlobalTypeRegistry::Snapshot::Snapshot()
{
}

GlobalTypeRegistry::Snapshot::~Snapshot()
{
}

GlobalTypeRegistry::Snapshot::Snapshot(const Snapshot& cpy)
{
	*this = std::move(cpy);
}

GlobalTypeRegistry::Snapshot::Snapshot(Snapshot&& mov)
{
	*this = std::move(mov);
}

GlobalTypeRegistry::Snapshot& GlobalTypeRegistry::Snapshot::operator=(const Snapshot& cpy)
{
	hashes = cpy.hashes;
	return *this;
}

GlobalTypeRegistry::Snapshot& GlobalTypeRegistry::Snapshot::operator=(Snapshot&& mov)
{
	hashes = std::move(mov.hashes);
	return *this;
}

void GlobalTypeRegistry::clear()
{
	modules.clear();
	_cachedSnapshotValue = std::nullopt;
}
