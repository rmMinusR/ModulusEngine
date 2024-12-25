#include "TemplateParam.hpp"

#include <cassert>

#include "TypeInfo.hpp"

std::string getName(const TemplateParam& param)
{
	if (const auto* concrete = std::get_if<TypeTemplateParam>(&param))
	{
		return concrete->name;
	}
	else
	{
		assert(false && "Unhandled template param type");
		return "";
	}
}

std::string getName(const TemplateParamValue& value)
{
	if (const auto* concrete = std::get_if<TypeTemplateParam::Value>(&value))
	{
		return (*concrete)->name.as_str();
	}
	else
	{
		assert(false && "Unhandled template param type");
		return "";
	}
}
