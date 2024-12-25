#pragma once

#include <vector>

#include "TemplateParam.hpp"


struct TemplateTypeInfo
{
public:
	// Dependent templates currently not supported
	// Value parameters currently not supported

	STIX_API TypeInfo instantiate(const std::vector<TemplateParam::Value>& parameters) const;

	STIX_API size_t minAlign() const;
	STIX_API size_t minSize() const;

private:
	struct Field
	{
		std::variant<TypeName, std::string> type; // 2nd is template param name
		std::string name;
	};

	TypeName name;
	std::vector<TemplateParam> templateParams;
	std::vector<Field> fields;
};
