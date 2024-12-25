#include "TemplateParam.hpp"

#include <cassert>

#include "TypeInfo.hpp"

std::string TemplateParam::getName() const
{
	if (const auto* concrete = std::get_if<TypeTemplateParam>(this))
	{
		return concrete->name;
	}
	else
	{
		assert(false && "Unhandled template param type");
		return "";
	}
}

std::string TemplateParam::Value::getName() const
{
	if (const auto* concrete = std::get_if<TypeTemplateParam::Value>(this))
	{
		return (*concrete)->name.as_str();
	}
	else
	{
		assert(false && "Unhandled template param type");
		return "";
	}
}
