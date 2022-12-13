#include <iostream>

#include "PluginCore.hpp"
#include "Plugin.hpp"

PLUGIN_API(bool) plugin_preInit(Plugin* context, EngineCore* engine)
{
    std::cout << "TestPlugin: plugin_preInit() called" << std::endl;
    return true;
}

PLUGIN_API(bool) plugin_init()
{
    std::cout << "TestPlugin: plugin_init() called" << std::endl;
    return true;
}

PLUGIN_API(void) plugin_cleanup()
{
    std::cout << "TestPlugin: plugin_cleanup() called" << std::endl;
}

PLUGIN_API(int) testExport()
{
    std::cout << "TestPlugin: testExport() called" << std::endl;
    return 42;
}
