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

	STIX_API void addField(TypeName type, const std::string& name);
	STIX_API void addField(TypeTemplateParam type, const std::string& name);

private:
	struct Field
	{
		std::variant<TypeName, std::string> type; // 2nd is template param name
		std::string name;
	};

	std::string name;
	std::vector<TemplateParam> templateParams;
	std::vector<Field> fields;
};
