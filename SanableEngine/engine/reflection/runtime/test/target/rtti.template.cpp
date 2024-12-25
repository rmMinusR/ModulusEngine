//Common includes
#include "TypeBuilder.hpp"

//Dependency includes
INCLUDE_DEPENDENCIES

//Grant access without friend decls everywhere
#include <public_cast.hpp>
PUBLIC_CAST_DECLS

//Clang will complain about inappropriate use of offsetof. Tests suggest it's wrong.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

void plugin_reportTypes(ModuleTypeRegistry* registry)
{
GENERATED_RTTI
}

#pragma clang diagnostic pop
