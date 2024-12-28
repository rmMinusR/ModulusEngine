#include "SyntheticTypeBuilder.hpp"

#include "ModuleTypeRegistry.hpp"

SyntheticTypeBuilder::SyntheticTypeBuilder(const std::string& name) :
	cursor(0)
{
	type.name = TypeName::createSynthetic(name);
}

SyntheticTypeBuilder::~SyntheticTypeBuilder()
{
}

size_t SyntheticTypeBuilder::nextAtAlign(size_t align) const
{
	size_t mask = align - 1; //Assumes align is power of two
	size_t roundedDown = cursor & ~mask;
	if (roundedDown == cursor) return cursor; //Already aligned
	else return roundedDown + align; //Not aligned, round up
}

void SyntheticTypeBuilder::addField_internal(const TypeName& fieldType, const std::string& name, size_t size, size_t align, MemberVisibility visibility)
{
	size_t loc = nextAtAlign(align);
	type.layout.fields.push_back(FieldInfo(size, loc, type.name, fieldType, name, visibility));
	type.layout.size = cursor = loc+size;
	type.layout.align = std::max(type.layout.align, align);
}

TypeInfo SyntheticTypeBuilder::finalize()
{
	return type;
}

void SyntheticTypeBuilder::registerType(ModuleTypeRegistry* reg)
{
	reg->types.push_back(finalize());
}
