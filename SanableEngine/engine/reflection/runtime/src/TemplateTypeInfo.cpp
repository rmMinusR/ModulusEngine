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

TypeInfo const* TemplateTypeInfo::resolveType(const PossiblyTemplatedType& ty, const std::vector<TemplateParamValue>& paramValues) const
{
	TypeInfo const* out = nullptr;

	if (auto const* concrete = std::get_if<TypeName>(&ty))
	{
		// Known constant type: just resolve
		out = concrete->resolve();
		assert(out && "Type specified but not could not resolve");
	}
	else if (auto const* paramIndex = std::get_if<size_t>(&ty))
	{
		if (*paramIndex < paramValues.size())
		{
			// Explicit param passed, use it
			out = std::get<TypeTemplateParam::Value>(paramValues[*paramIndex]);
		}
		else
		{
			// No explicit param passed, use default param
			out = std::get<TypeTemplateParam>(templateParams[*paramIndex]).defaultValue;
			assert(out && "Template parameter lacks default value, and none was provided");
		}
	}
	else
	{
		assert(false && "Unhandled template param type");
	}

	return out;
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

	// Put parents
	for (const auto& p : parents)
	{
		builder.addParent(*resolveType(p.type, paramValues), p.visibility, ParentInfo::Virtualness::NonVirtual);
	}

	// Put fields
	for (const auto& fi : fields)
	{
		builder.addField(*resolveType(fi.type, paramValues), fi.name);
	}

	return builder.finalize();
}

size_t TemplateTypeInfo::minAlign() const
{
	size_t align = 1;

	// Walk parents
	for (const auto& p : parents)
	{
		if (TypeName const* concrete = std::get_if<TypeName>(&p.type))
		{
			TypeInfo const* concreteFullType = concrete->resolve();
			assert(concreteFullType);
			if (concreteFullType)
			{
				align = std::max(align, concreteFullType->layout.align);
			}
		}
	}

	// Walk fields
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

	// Walk parents
	for (const auto& p : parents)
	{
		TypeName const* concrete = std::get_if<TypeName>(&p.type);
		if (concrete)
		{
			TypeInfo const* concreteFullType = concrete->resolve();
			assert(concreteFullType);
			if (concreteFullType)
			{
				size_t align = concreteFullType->layout.align;
				if (size & (align - 1))
				{
					// Fixup alignment
					// Assumes power of two
					size = size & ~(align - 1);
					size += align;
				}
				size += concreteFullType->layout.size;
			}
			else size++; // Unloaded/incomplete type: must be at least one byte
		}
		else size++; // Template type: must be at least one byte
	}

	// Walk fields
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
	return addTypeParam(name, nullptr);
}

const TypeTemplateParam& TemplateTypeInfo::addTypeParam(const std::string& name, const TypeInfo* defaultValue)
{
	assert(std::find_if(templateParams.begin(), templateParams.end(), [&](const TemplateParam& i) { return getName(i) == name; }) == templateParams.end());

	return std::get<TypeTemplateParam>(
		templateParams.emplace_back(TypeTemplateParam{ name, defaultValue })
	);
}

void TemplateTypeInfo::addField(TypeName knownType, const std::string& name)
{
	fields.push_back(Field{ knownType, name });
}

void TemplateTypeInfo::addField(TypeTemplateParam type, const std::string& name)
{
	// Lookup what index this parameter is
	const auto it = std::find_if(templateParams.begin(), templateParams.end(), [=](const TemplateParam& p) { return std::get<TypeTemplateParam>(p).name == type.name; });
	assert(it != templateParams.end());
	size_t paramIndex = it - templateParams.begin();

	fields.push_back(Field{ paramIndex, name });
}

void TemplateTypeInfo::addParent(TypeName knownType, MemberVisibility visibility)
{
	parents.push_back(Parent{ knownType, visibility });
}

void TemplateTypeInfo::addParent(TypeTemplateParam type, MemberVisibility visibility)
{
	// Lookup what index this parameter is
	const auto it = std::find_if(templateParams.begin(), templateParams.end(), [=](const TemplateParam& p) { return std::get<TypeTemplateParam>(p).name == type.name; });
	assert(it != templateParams.end());
	size_t paramIndex = it - templateParams.begin();

	parents.push_back(Parent{ paramIndex, visibility });
}
