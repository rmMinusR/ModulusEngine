#include "TemplateTypeInfo.hpp"

#include <cassert>
#include <sstream>

#include "SyntheticTypeBuilder.hpp"

TemplateTypeInfo::TemplateTypeInfo(std::string name) :
	name(name)
{
}

TemplateTypeInfo::~TemplateTypeInfo()
{
}

TypeInfo TemplateTypeInfo::instantiate(const std::vector<TemplateParamValue>& paramValues) const
{
	assert(paramValues.size() <= templateParams.size()); // Can't over-pass

	// Assemble name
	std::stringstream tmp;
	tmp << name << "<";
	for (size_t i = 0; i < paramValues.size(); ++i)
	{
		if (i != 0) tmp << ", ";
		tmp << getName(paramValues[i]);
	}
	tmp << ">";
	SyntheticTypeBuilder builder(tmp.str());

	// Put fields
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
			const auto it = std::find_if(templateParams.begin(), templateParams.end(), [=](const TemplateParam& p) { return std::get<TypeTemplateParam>(p).name == *paramName; });
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
				if (size & (align-1))
				{
					// Fixup alignment
					// Assumes power of two
					size = size & ~(align-1);
					size += align;
				}
				size += concreteFullType->layout.size;
			}
			else size++; // Unloaded/incomplete type: must be at least one byte
		}
		else size++; // Template type: must be at least one byte
	}
	return std::max(size, (size_t)1);
}

const TypeTemplateParam& TemplateTypeInfo::addTypeParam(const std::string& name)
{
	return std::get<TypeTemplateParam>(
		templateParams.emplace_back(TypeTemplateParam{ name })
	);
}

const TypeTemplateParam& TemplateTypeInfo::addTypeParam(const std::string& name, const TypeInfo* defaultValue)
{
	return std::get<TypeTemplateParam>(
		templateParams.emplace_back(TypeTemplateParam{ name, defaultValue })
	);
}

void TemplateTypeInfo::addField(TypeName type, const std::string& name)
{
	fields.push_back({ type, name });
}

void TemplateTypeInfo::addField(TypeTemplateParam type, const std::string& name)
{
	fields.push_back({ type.name, name });
}
