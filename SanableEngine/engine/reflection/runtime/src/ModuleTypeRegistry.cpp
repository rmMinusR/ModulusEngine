#include "ModuleTypeRegistry.hpp"

void ModuleTypeRegistry::doLateBinding()
{
	for (TypeInfo& i : types) i.doLateBinding(this);
}

TypeInfo const* ModuleTypeRegistry::lookupType(const TypeName& name) const
{
	for (const TypeInfo& i : types) if (i.name == name) return &i;
	return nullptr;
}

const std::vector<TypeInfo>& ModuleTypeRegistry::getTypes() const
{
	return types;
}

TypeInfo const* ModuleTypeRegistry::snipeType(void* obj, size_t size, TypeInfo const* hint) const
{
	for (const TypeInfo& i : types)
	{
		if (i.layout.size <= size && i.layout.matchesExact(obj)) return &i;
	}
	return nullptr;
}

ModuleTypeRegistry::ModuleTypeRegistry()
{

}

ModuleTypeRegistry::~ModuleTypeRegistry()
{

}

ModuleTypeRegistry::ModuleTypeRegistry(const ModuleTypeRegistry& cpy)
{
	*this = cpy;
}

ModuleTypeRegistry::ModuleTypeRegistry(ModuleTypeRegistry&& mov)
{
	*this = std::move(mov);
}

ModuleTypeRegistry& ModuleTypeRegistry::operator=(const ModuleTypeRegistry& cpy)
{
	types = cpy.types;
	for (TypeInfo& ty : types) ty.layout.ownModule = this;
	return *this;
}

ModuleTypeRegistry& ModuleTypeRegistry::operator=(ModuleTypeRegistry&& mov)
{
	types = std::move(mov.types);
	for (TypeInfo& ty : types) ty.layout.ownModule = this;
	return *this;
}
