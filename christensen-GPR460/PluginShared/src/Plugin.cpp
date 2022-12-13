#include "Plugin.hpp"

#if __EMSCRIPTEN__
#include <dlfcn.h>
#endif

#include <cassert>

#include "PluginCore.hpp"

__stdcall Plugin::Plugin(const std::filesystem::path& path) :
	path(path),
	status(Status::NotLoaded)
{
#ifdef _WIN32
	dll = (HMODULE)INVALID_HANDLE_VALUE;
#endif
#ifdef __EMSCRIPTEN__
	dll = nullptr;
#endif
}

Plugin::~Plugin()
{
	assert(!_dllGood() && status == Status::NotLoaded);
}

void* Plugin::getSymbol(const char* name) const
{
	assert(_dllGood());
#ifdef _WIN32
	return GetProcAddress(dll, name);
#endif
#ifdef __EMSCRIPTEN__
	return dlsym(dll, name);
#endif
}

bool Plugin::_dllGood() const
{
#ifdef _WIN32
	return dll != INVALID_HANDLE_VALUE;
#endif
#ifdef __EMSCRIPTEN__
	return dll;
#endif
}

void Plugin::loadDLL()
{
	assert(status == Status::NotLoaded);
	assert(!_dllGood());

#ifdef _WIN32
	dll = LoadLibrary(path.c_str());
#endif
#ifdef __EMSCRIPTEN__
	dll = dlopen(path.c_str(), RTLD_LAZY);
#endif
	assert(_dllGood());
	
	status = Status::DllLoaded;
}

void Plugin::preInit(EngineCore* engine)
{
	assert(status == Status::DllLoaded);
	assert(_dllGood());
	
	fp_plugin_preInit func = (fp_plugin_preInit)getSymbol("plugin_preInit");
	assert(func);
	bool success = func(this, engine);
	assert(success);

	status = Status::Registered;
}

void Plugin::init()
{
	assert(status == Status::Registered);
	assert(_dllGood());

	fp_plugin_init func = (fp_plugin_init)getSymbol("plugin_init");
	assert(func);
	bool success = func();
	assert(success);

	status = Status::Hooked;
}

void Plugin::cleanup()
{
	assert(status == Status::Hooked);
	assert(_dllGood());

	fp_plugin_cleanup func = (fp_plugin_cleanup)getSymbol("plugin_cleanup");
	assert(func);
	func();

	status = Status::Registered;
}

void Plugin::unloadDLL()
{
	assert(status == Status::DllLoaded || status == Status::Registered);
	assert(_dllGood());

#ifdef _WIN32
	BOOL success = FreeLibrary(dll);
	assert(success);
	dll = (HMODULE)INVALID_HANDLE_VALUE;
#endif
#ifdef __EMSCRIPTEN__
	int failure = dlclose(dll);
	assert(!failure);
	dll = nullptr;
#endif

	status = Status::NotLoaded;
}