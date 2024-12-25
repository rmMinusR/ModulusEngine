#pragma once

#include "TypeInfo.hpp"

class ModuleTypeRegistry;

class SyntheticTypeBuilder
{
	TypeInfo type;
	size_t cursor; //Next free byte
	size_t nextAtAlign(size_t align) const;

	STIX_API void addField_internal(const TypeName& fieldType, const std::string& name, size_t size, size_t align, MemberVisibility visibility);
	//STIX_API void addParent_internal(const TypeName& parent, size_t parentSize, size_t parentAlign, MemberVisibility visibility, ParentInfo::Virtualness virtualness);
public:
	STIX_API SyntheticTypeBuilder(const std::string& name);
	STIX_API ~SyntheticTypeBuilder();

	template<typename T>
	void addField(const std::string& name, MemberVisibility visibility = MemberVisibility::Public)
	{
		addField_internal(TypeName::create<T>(), name, sizeof(T), alignof(T), visibility);
	}

	void addField(const TypeInfo& type, const std::string& name, MemberVisibility visibility = MemberVisibility::Public)
	{
		addField_internal(type.name, name, type.layout.size, type.layout.align, visibility);
	}

	STIX_API TypeInfo finalize();
	STIX_API void registerType(ModuleTypeRegistry* reg); //Useful for test mocking
};
