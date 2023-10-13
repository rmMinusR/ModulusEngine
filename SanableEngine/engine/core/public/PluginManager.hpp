#pragma once

#include "dllapi.h"

#include <vector>
#include <EngineCoreReflectionHooks.hpp>

#include "Plugin.hpp"

class ModuleTypeRegistry;
class EngineCore;

class PluginManager
{
	SANABLE_REFLECTION_HOOKS

private:
	EngineCore* const engine;
	std::vector<Plugin*> plugins;
	
	void discoverAll(const std::filesystem::path& pluginsFolder);
	void load(const std::wstring& dllPath);
	void hookAll(bool firstRun);
	void unhookAll(bool shutdown);
	void unloadAll();

	void reloadAll();

	PluginManager(EngineCore* engine);
	~PluginManager();

	friend class EngineCore;

public:
	//Plugin const* getPlugin(const std::wstring& name);
};
