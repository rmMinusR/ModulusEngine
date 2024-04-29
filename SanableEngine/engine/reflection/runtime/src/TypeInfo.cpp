#include "TypeInfo.hpp"

#include <cassert>
#include <sstream>

#include "GlobalTypeRegistry.hpp"

TypeInfo::TypeInfo()
{

}

TypeInfo::~TypeInfo()
{
}

TypeInfo::TypeInfo(const TypeInfo& cpy)
{
	*this = cpy; //Defer to operator=
}

TypeInfo::TypeInfo(TypeInfo&& mov)
{
	*this = mov; //Defer to operator=
}

TypeInfo& TypeInfo::operator=(const TypeInfo & cpy)
{
	this->name         = cpy.name;
	this->layout       = cpy.layout;
	this->capabilities = cpy.capabilities;

	return *this;
}

TypeInfo& TypeInfo::operator=(TypeInfo&& mov)
{
	this->name         = std::move(mov.name);
	this->layout       = std::move(mov.layout);
	this->capabilities = std::move(mov.capabilities);

	return *this;
}

bool TypeInfo::isValid() const
{
	return layout.size != 0
		&& name.isValid();
}

bool TypeInfo::isLoaded() const
{
	assert(isValid());
	return GlobalTypeRegistry::lookupType(name);
}

bool TypeInfo::tryRefresh()
{
	if (isLoaded())
	{
		//Overwrite values
		*this = *GlobalTypeRegistry::lookupType(name);
		return true;
	}
	else return false;
}

const FieldInfo* TypeInfo::Layout::getField(const std::string& name, MemberVisibility visibilityFlags, bool includeInherited) const
{
	//Search own fields
	auto it = std::find_if(fields.begin(), fields.end(), [&](const FieldInfo& fi) { return fi.name == name; });
	if (it != fields.end() && ((int)it->visibility & (int)visibilityFlags)) return &(*it);
	
	//Search parent fields
	if (includeInherited)
	{
		for (const ParentInfo& parent : parents)
		{
			if ((int)parent.visibility & (int)visibilityFlags)
			{
				const FieldInfo* out = parent.typeName.resolve()->layout.getField(name, visibilityFlags, true);
				if (out) return out;
			}
		}
	}

	//Found nothing
	return nullptr;
}

std::optional<ParentInfo> TypeInfo::Layout::getParent_internal(const TypeName& ownType, const TypeName& name, MemberVisibility visibilityFlags, bool includeInherited, bool makeComplete) const
{
	//If referring to self, nothing to do
	if (name == ownType) return std::nullopt;

	//Check immediate parents first
	for (const ParentInfo& parent : parents)
	{
		if (parent.typeName == name) return parent;
	}

	//Recurse if allowed
	if (includeInherited)
	{
		for (const ParentInfo& parent : parents)
		{
			std::optional<ParentInfo> baseOfVbase = parent.typeName.resolve()->getParent(name, visibilityFlags, includeInherited);
			if (baseOfVbase.has_value() && baseOfVbase.value().virtualness == ParentInfo::Virtualness::NonVirtual)
			{
				if (makeComplete)
				{
					baseOfVbase.value().owner = ownType;
					baseOfVbase.value().offset += parent.offset;
				}
				return baseOfVbase.value();
			}
		}
	}

	//Not a parent
	return std::nullopt;
}

std::optional<ParentInfo> TypeInfo::getParent(const TypeName& name, MemberVisibility visibilityFlags, bool includeInherited, bool makeComplete) const
{
	return layout.getParent_internal(this->name, name, visibilityFlags, includeInherited, makeComplete);
}

void TypeInfo::Layout::walkFields(std::function<void(const FieldInfo&)> visitor, MemberVisibility visibilityFlags, bool includeInherited) const
{
	//Recurse into parents first
	//C++ treats parents as fields placed before the first explicit field
	if (includeInherited)
	{
		for (const ParentInfo& parent : parents)
		{
			if ((int)parent.visibility & (int)visibilityFlags)
			{
				const TypeInfo* parentType = parent.typeName.resolve();
				if (parentType)
				{
					//Can't walk what isn't loaded
					parentType->layout.walkFields(
						visitor,
						visibilityFlags,
						true
					);
				}
				else
				{
					printf("ERROR: %s was not loaded. Cannot walk all fields.\n", parent.typeName.c_str());
				}
			}
		}
	}

	//Walk own fields
	for (const FieldInfo& field : fields)
	{
		if ((int)field.visibility & (int)visibilityFlags)
		{
			visitor(field);
		}
	}
}

void TypeInfo::Layout::vptrJam(void* obj) const
{
	assert(!byteUsage.empty());
	if (!implicitValues.empty())
	{
		//Write captured constants from implicitly generated fields
		for (size_t i = 0; i < size; ++i)
		{
			if (byteUsage[i] == ByteUsage::ImplicitConst) static_cast<char*>(obj)[i] = implicitValues[i];
		}
	}
}

void* TypeInfo::Layout::upcast(void* obj, const TypeName& parentTypeName) const
{
	std::optional<ParentInfo> parent = getParent_internal(TypeName(), parentTypeName);
	if (parent.has_value()) return upcast(obj, parent.value()); //Defer
	else return nullptr; //Not a parent
}

void* TypeInfo::Layout::upcast(void* obj, const ParentInfo& parentType) const
{
	//assert(parentType.owner == this->name); //TODO re-add safety assert
	return ((char*)obj)+parentType.offset;
}

bool TypeInfo::Layout::matchesExact(void* obj) const
{
	assert(!byteUsage.empty());
	assert(!implicitValues.empty());

	for (size_t i = 0; i < size; ++i)
	{
		if (byteUsage[i] == ByteUsage::ImplicitConst && reinterpret_cast<char*>(obj)[i] != implicitValues[i]) return false; //If implicit const detected, value must match
	}

	return true;
}

void TypeInfo::doLateBinding()
{
	//Deferred from captureCDO: Mark all fields as used
	assert(!layout.byteUsage.empty());
	layout.walkFields(
		[&](const FieldInfo& fi) {
			ptrdiff_t root = fi.offset + (ptrdiff_t)this->layout.upcast(nullptr, fi.owner);
			assert(layout.byteUsage[root] != Layout::ByteUsage::ImplicitConst && "Attempted to overwrite data (usage clobbering)");
			memset(layout.byteUsage.data()+root, (uint8_t)Layout::ByteUsage::ExplicitField, fi.size);
		},
		MemberVisibility::All,
		true
	);
}

void TypeInfo::create_internalFinalize()
{
	layout.byteUsage.resize(layout.size);
	memset(layout.byteUsage.data(), (uint8_t)Layout::ByteUsage::Unknown, layout.size);
}
