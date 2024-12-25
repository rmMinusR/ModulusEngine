#pragma once

#include <variant>
#include <string>

#include "dllapi.h"

#include "TypeName.hpp"


struct TypeTemplateParam
{
	using Value = TypeInfo const*;

	std::string name; // 0-length for anonymous parameters
	Value defaultValue = nullptr; // Invalid if no default specified
};

#define _XM_FOREACH_TEMPLATE_PARAM_TYPE(MACRO) MACRO(TypeTemplateParam)

#define _EXPAND(X) X
#define _GET_VALUE(X) X::Value
using TemplateParam = std::variant<_XM_FOREACH_TEMPLATE_PARAM_TYPE(_EXPAND)>;
using TemplateParamValue = std::variant<_XM_FOREACH_TEMPLATE_PARAM_TYPE(_GET_VALUE)>;
#undef _EXPAND
#undef _GET_VALUE

STIX_API std::string getName(const TemplateParam&);
STIX_API std::string getName(const TemplateParamValue&);
