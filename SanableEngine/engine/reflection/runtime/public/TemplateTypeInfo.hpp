#pragma once

#include <vector>

#include "TemplateParam.hpp"
#include "TypeInfo.hpp"


struct TemplateTypeInfo
{
public:
	STIX_API TemplateTypeInfo(std::string name);
	STIX_API ~TemplateTypeInfo();

	// Result getters

	STIX_API TypeInfo instantiate(const std::vector<TemplateParamValue>& parameters) const;
	STIX_API size_t minAlign() const;
	STIX_API size_t minSize() const;

	// Template params

	STIX_API const TypeTemplateParam& addTypeParam(const std::string& name);
	STIX_API const TypeTemplateParam& addTypeParam(const std::string& name, const TypeInfo* defaultValue);
	// Dependent templates currently not supported
	// Value parameters currently not supported

	// Fields

	STIX_API void addField(TypeName knownType, const std::string& name);
	STIX_API void addField(TypeTemplateParam type, const std::string& name);
	STIX_API void addParent(TypeName knownType, MemberVisibility visibility = MemberVisibility::Public); // Virtual inheritance currently not supported
	STIX_API void addParent(TypeTemplateParam type, MemberVisibility visibility = MemberVisibility::Public);

private:
	using PossiblyTemplatedType = std::variant<TypeName, size_t>; // 2nd is template param index
	TypeInfo const* resolveType(const PossiblyTemplatedType& ty, const std::vector<TemplateParamValue>& paramValues) const;

	struct Parent
	{
		PossiblyTemplatedType type;
		MemberVisibility visibility = MemberVisibility::Public;
	};

	struct Field
	{
		PossiblyTemplatedType type;
		std::string name;
	};

	std::string name;
	std::vector<TemplateParam> templateParams;
	std::vector<Parent> parents;
	std::vector<Field> fields;
};
