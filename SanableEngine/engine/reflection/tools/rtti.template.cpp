//Common includes
#include "PluginCore.hpp"
#include "TypeBuilder.hpp"

//Dependency includes
INCLUDE_DEPENDENCIES

//Clang will complain about inappropriate use of offsetof. Tests suggest it's wrong.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

PLUGIN_C_API(void) plugin_reportTypes(ModuleTypeRegistry* registry)
{
GENERATED_RTTI
}

#pragma clang diagnostic pop
