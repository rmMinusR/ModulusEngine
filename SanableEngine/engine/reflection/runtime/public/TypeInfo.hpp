#pragma once

#include <string>
#include <functional>
#include <optional>

#include "dllapi.h"

#include "ThunkUtils.hpp"
#include "TypeName.hpp"
#include "ParentInfo.hpp"
#include "FieldInfo.hpp"
#include "Function.hpp"

class TypeBuilder;
class ModuleTypeRegistry;

/// <summary>
/// For cases where we cannot use C++ builtin type_info.
/// Mainly whenever the owning DLL is unloaded.
/// </summary>
struct TypeInfo
{
private:
	size_t hash;
	friend class TypeBuilder;
	friend struct std::hash<TypeInfo>;
public:
	TypeName name;

	struct Layout
	{
		size_t size = 0;
		size_t align = 0;
	private:
		std::vector<ParentInfo> parents;
		std::vector<FieldInfo> fields; //NO TOUCHY! Use walkFields instead, which will also handle parent recursion.

		ModuleTypeRegistry* ownModule = nullptr; // VERY stupid fix for out-of-order registration
		friend class ModuleTypeRegistry;

		/// <summary>
		/// How is each byte used?
		/// </summary>
		enum class ByteUsage : uint8_t
		{
			Unknown = 0,
			ExplicitField = 'f',
			ImplicitConst = 'i',
			Padding = 'p'
		};

		std::vector<ByteUsage> byteUsage; //Each value maps directly onto implicitValues' corresponding byte
		std::vector<char> implicitValues; //Implicitly generated members (read: vptrs)

		/// <summary>
		/// Look up a parent by name
		/// </summary>
		/// <param name="name">Name of parent</param>
		/// <param name="visibilityFlags">What parents should be visible or ignored</param>
		/// <param name="includeInherited">If true, recurse into parents</param>
		/// <param name="makeComplete">If true, and also includeInherited, grandparent ParentInfos will be adjusted to be valid for this type</param>
		std::optional<ParentInfo> getParent_internal(const TypeName& ownType, const TypeName& name,
												MemberVisibility visibilityFlags = MemberVisibility::Public,
												bool includeInherited = true,
												bool makeComplete = true) const;
	public:
		/// <summary>
		/// Look up a field by name. Returns nullptr if not found.
		/// </summary>
		STIX_API const FieldInfo* getField(const std::string& name,
												MemberVisibility visibilityFlags = MemberVisibility::Public,
												bool includeInherited = true) const;

		/// <summary>
		/// Visit every field in this type matching the given query.
		/// </summary>
		/// <param name="visitor">Function to run on every FieldInfo</param>
		/// <param name="visibilityFlags">What members/parents should be visible or ignored</param>
		/// <param name="includeInherited">Include fields inherited from parents?</param>
		STIX_API void walkFields(std::function<void(const FieldInfo&)> visitor,
										MemberVisibility visibilityFlags = MemberVisibility::Public,
										bool includeInherited = true) const;


		struct ImplicitInfo //TEMP communications helper
		{
			ptrdiff_t offset;
			size_t size;
			const uint8_t* data; //nullptr if padding
		};

		/// <summary>
		/// Visit every implicit constant in this type.
		/// </summary>
		STIX_API void walkImplicits(std::function<void(const ImplicitInfo&)> visitor) const;

		/// <summary>
		/// Update vtable pointers on the given object instance.
		/// </summary>
		/// <param name="obj">Object to be updated</param>
		STIX_API void vptrJam(void* obj) const;

		/// <summary>
		/// Cast to a parent. Returns null if no parent found.
		/// </summary>
		/// <param name="obj">Object to cast</param>
		STIX_API void* upcast(void* obj, const TypeName& parentTypeName) const;
		STIX_API void* upcast(void* obj, const ParentInfo& parentType) const;

		/// <summary>
		/// Check if this type inherits from the given type
		/// </summary>
		/// <param name="type">Type to check against</param>
		/// <param name="grandparents">If false, don't check grandparents</param>
		STIX_API bool isDerivedFrom(const TypeName& type, bool grandparents = true) const;

		/// <summary>
		/// Test if the given object matches this type exactly. Does not test if object's type is derived from this.
		/// Requires: captured ctor, virtual object
		/// </summary>
		STIX_API bool matchesExact(void* obj) const;

		friend class TypeBuilder; //Only thing allowed to touch all member data.
		friend struct TypeInfo;
	} layout;

	struct Capabilities
	{
		dtor_t rawDtor; //Not guaranteed to be present, null check before calling

		struct MemberFuncRecord
		{
			stix::MemberFunction fn;
			std::string name;
			MemberVisibility visibility;
			bool isVirtual;
		};

		struct StaticFuncRecord
		{
			stix::StaticFunction fn;
			std::string name;
			MemberVisibility visibility;
		};

		struct ConstructorRecord
		{
			stix::StaticFunction thunk;
			MemberVisibility visibility;
		};

		struct DestructorRecord
		{
			std::optional<stix::MemberFunction> ctor; //nullopt if we couldn't capture it
			MemberVisibility visibility;
			bool isVirtual;
		};
		
		//These do not include parents' funcs/ctors/dtors. Should they?
		std::vector<MemberFuncRecord> memberFuncs;
		std::vector<StaticFuncRecord> staticFuncs;
		std::vector<ConstructorRecord> constructors;
		std::optional<DestructorRecord> richDtor; //Detects implicit dtors using standard C++. If nullopt, destructor is deleted.

		STIX_API const stix::MemberFunction* getMemberFunction(const std::string& name, MemberVisibility visibility = MemberVisibility::Public) const;
		STIX_API const stix::MemberFunction* getMemberFunction(const std::string& name, const std::vector<TypeName>& paramTypes, MemberVisibility visibility = MemberVisibility::Public) const;
		STIX_API const stix::StaticFunction* getStaticFunction(const std::string& name, MemberVisibility visibility = MemberVisibility::Public) const;
		STIX_API const stix::StaticFunction* getStaticFunction(const std::string& name, const std::vector<TypeName>& paramTypes, MemberVisibility visibility = MemberVisibility::Public) const;

	} capabilities;

public:
	STIX_API TypeInfo();
	STIX_API ~TypeInfo();

	STIX_API TypeInfo(const TypeInfo& cpy);
	STIX_API TypeInfo(TypeInfo&& mov);
	STIX_API TypeInfo& operator=(const TypeInfo& cpy);
	STIX_API TypeInfo& operator=(TypeInfo&& mov);

	/// <summary>
	/// Check if this type has data (ie. hasn't been empty-constructed).
	/// Does NOT indicate whether using instances will cause errors.
	/// </summary>
	/// <returns></returns>
	STIX_API bool isValid() const;

	STIX_API bool isDummy() const;

	/// <summary>
	/// Check if this type is currently loaded.
	/// If so, instances can be used without causing errors.
	/// </summary>
	STIX_API bool isLoaded() const;

	/// <summary>
	/// Update from live copy in GlobalTypeRegistry, if one is present.
	/// </summary>
	/// <returns>Whether a live copy was present</returns>
	STIX_API bool tryRefresh();

	/// <summary>
	/// Look up a parent by name
	/// </summary>
	/// <param name="name">Name of parent</param>
	/// <param name="visibilityFlags">What parents should be visible or ignored</param>
	/// <param name="includeInherited">If true, recurse into parents</param>
	/// <param name="makeComplete">If true, and also includeInherited, grandparent ParentInfos will be adjusted to be valid for this type</param>
	STIX_API std::optional<ParentInfo> getParent(const TypeName& name,
											MemberVisibility visibilityFlags = MemberVisibility::Public,
											bool includeInherited = true,
											bool makeComplete = true) const;

	/// <summary>
	/// INTERNAL USE ONLY. Currently used to finalize byteUsage, since we need to be able to look up our parents' fields.
	/// </summary>
	STIX_INTERNAL( void doLateBinding(ModuleTypeRegistry* ownModule); )

private:
	/// <summary>
	/// INTERNAL USE ONLY. Currently used to set up byte usage mask.
	/// </summary>
	STIX_API void create_internalFinalize();

public:
	/// <summary>
	/// INTERNAL USE ONLY by TypeBuilder and TypedMemoryPool.
	/// </summary>
	template<typename TObj>
	static TypeInfo createDummy()
	{
		TypeInfo out;
		out.name = TypeName::create<TObj>();
		out.layout.size = sizeof(TObj);
		out.layout.align = alignof(TObj);
		out.capabilities.rawDtor = thunk_utils<TObj>::dtor;
		out.create_internalFinalize();
		out.hash = 0;
		return out;
	}

};

template<> struct std::hash<TypeInfo>
{
	inline size_t operator()(const TypeInfo& t) { return t.hash; }
};
