#include "TemplateTypeInfo.hpp"

#include <cassert>
#include <sstream>

#include "TypeInfo.hpp"
#include "SyntheticTypeBuilder.hpp"

TypeInfo TemplateTypeInfo::instantiate(const std::vector<TemplateParam::Value>& paramValues) const
{
	std::stringstream tmp;
	tmp << name.as_str() << "<";
	for (size_t i = 0; i < paramValues.size(); ++i)
	{
		if (i != 0) tmp << ", ";
		tmp << paramValues[i].getName();
	}
	tmp << ">";
	SyntheticTypeBuilder builder(tmp.str());

	for (const auto& fi : fields)
	{
		TypeInfo const* type = nullptr;

		if (auto const* concrete = std::get_if<TypeName>(&fi.type))
		{
			// Known constant type: just resolve
			type = concrete->resolve();
			assert(type && "Type specified but not could not resolve");
		}
		else if (auto const* paramName = std::get_if<std::string>(&fi.type))
		{
			// Lookup what index this parameter is
			const auto it = std::find_if(templateParams.begin(), templateParams.end(), [=](const TemplateParam& p) { return p.getName() == *paramName; });
			assert(it != templateParams.end());
			size_t paramIndex = it - templateParams.begin();

			if (paramIndex < paramValues.size())
			{
				// Explicit param passed, use it
				type = std::get<TypeTemplateParam::Value>(paramValues[paramIndex]);
			}
			else
			{
				// No explicit param passed, use default param
				type = std::get<TypeTemplateParam>(templateParams[paramIndex]).defaultValue;
				assert(type && "Template parameter lacks default value, and none was provided");
			}
		}
		else
		{
			assert(false && "Unhandled template param type");
		}

		builder.addField(*type, fi.name);
	}

	return builder.finalize();
}

size_t TemplateTypeInfo::minAlign() const
{
	size_t align = 1;
	for (const auto& fi : fields)
	{
		if (TypeName const* concrete = std::get_if<TypeName>(&fi.type))
		{
			TypeInfo const* concreteFullType = concrete->resolve();
			assert(concreteFullType);
			if (concreteFullType)
			{
				align = std::max(align, concreteFullType->layout.align);
			}
		}
	}
	return align;
}

size_t TemplateTypeInfo::minSize() const
{
	size_t size = 0;
	for (const auto& fi : fields)
	{
		TypeName const* concrete = std::get_if<TypeName>(&fi.type);
		if (concrete)
		{
			TypeInfo const* concreteFullType = concrete->resolve();
			assert(concreteFullType);
			if (concreteFullType)
			{
				size_t align = concreteFullType->layout.align;
				if (size % align != 0)
				{
					// Not aligned: round up
					size = size - (size % align) + align;
				}
				size += concreteFullType->layout.size;
			}
		}
	}
	return std::max(size, (size_t)1);
}
