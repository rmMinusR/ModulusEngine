#include "TypeName.hpp"

#include <cassert>

#include "GlobalTypeRegistry.hpp"
#include "FundamentalTypes.hpp"

bool TypeName::strip_leading(std::string& str, const std::string& phrase)
{
    //Can't strip if too small
    if (str.length() < phrase.length()) return false;

    //Does str start with phrase?
    if (!strncmp(str.c_str(), phrase.c_str(), phrase.length()))
    {
        str = str.substr(phrase.length(), str.length()-phrase.length());
        return true;
    }

    return false;
}

bool TypeName::strip_trailing(std::string& str, const std::string& phrase)
{
    //Can't strip if too small
    if (str.length() < phrase.length()) return false;

    //Does str end with phrase?
    char const* endPart = str.c_str() + str.length() - phrase.length();
    if (!strncmp(endPart, phrase.c_str(), phrase.length()))
    {
        str = str.substr(0, str.length()-phrase.length());
        return true;
    }

    return false;
}

bool TypeName::strip(std::string& str, const std::string& phrase, bool spacePadded)
{
    constexpr size_t bufSize = 256;
    assert(bufSize >= phrase.length()+2);

    char phrasePadded[bufSize];
    phrasePadded[0] = ' ';
    phrasePadded[phrase.length()+1] = ' ';
    memcpy(phrasePadded+1, phrase.c_str(), phrase.length());

    bool ret = strip_leading(str, !spacePadded?phrase:std::string(phrasePadded+1, phrase.size()+1));
    ret |=    strip_trailing(str, !spacePadded?phrase:std::string(phrasePadded  , phrase.size()+1));
    return ret;
}

TypeName::hash_t TypeName::makeHash(const std::string& str)
{
    //DJB2 string hash
    hash_t hash = 5381;
        
    for (int i = 0; i < str.length(); ++i)
    {
        //hash = ((hash << 5) + hash) + c;
        hash = hash * 33 ^ str[i];
    }

    return hash;
}

TypeName::TypeName() :
    name(""),
    nameHash(0)
{
}

TypeName::TypeName(const std::string& _name, Flags flags)
{
    name = _name;
    nameHash = makeHash(name);
    this->flags = flags;
}

const char* TypeName::incomplete_ref_literal = "!!incomplete type!!&";
TypeName TypeName::incomplete_ref()
{
    return TypeName(incomplete_ref_literal, Flags::Incomplete);
}

std::optional<TypeName> TypeName::cvUnwrap() const
{
    std::string unwrappedName = name;
    bool unwrappable = false;
    if (this->dereference().has_value())
    {
        //If it's a pointer type, only allow taking trailing symbols
        //Leading const keyword only applies to innermost type

        //We don't know the order in which const/volatile are arranged
        unwrappable |= strip_trailing(unwrappedName, " const");
        unwrappable |= strip_trailing(unwrappedName, " volatile");
        unwrappable |= strip_trailing(unwrappedName, " const");
    }
    else
    {
        unwrappable |= strip(unwrappedName, "const", true);
        unwrappable |= strip(unwrappedName, "volatile", true);
        unwrappable |= strip(unwrappedName, "const", true);
    }
    return unwrappable ? std::make_optional(TypeName(unwrappedName, flags)) : std::nullopt;
}

std::optional<TypeName> TypeName::dereference() const
{
    //FIXME this won't work with function pointers. Too bad!

    size_t index = name.find_last_of("*");
    if (index == std::string::npos) return std::nullopt;

    //Skip const/volatile/__ptr64, but make sure we aren't doing anything funny like going into a template's <>
    for (size_t i = index+1; i < name.size(); ++i)
    {
        char c = name.at(i);
        if (!isalnum(c) && !isspace(c) && c != '_') return std::nullopt;
    }
    
    std::string unwrappedName = name.substr(0, index);
    strip_trailing(unwrappedName, " ");
    return TypeName(unwrappedName, flags);
}

bool TypeName::isDataPtr() const
{
    auto ptrTokIndex = name.find_last_of("*");
    auto templateCloseTokIndex = name.find_last_of(">"); //-1 if not found
    auto fnArgCloseTokIndex = name.find_last_of(")"); //-1 if not found
    return ptrTokIndex > templateCloseTokIndex && ptrTokIndex > fnArgCloseTokIndex;
}

bool TypeName::isComposite() const
{
    return !isFundamental() && !isDataPtr();
}

bool TypeName::isFundamental() const
{
    for (size_t i = 0; i < fundamentalTypes_names_sz; ++i)
    {
        if (name == fundamentalTypes_names[i]) return true;
    }
    return false;
}

bool TypeName::isValid() const
{
    return !name.empty();
}

TypeInfo const* TypeName::resolve(ModuleTypeRegistry* moduleHint) const
{
    TypeInfo const* out = nullptr;
    if (moduleHint && ( out = moduleHint->lookupType(*this) )) return out; // Try hinted TU first
    return GlobalTypeRegistry::lookupType(*this); // Then try all
}

bool TypeName::operator==(const TypeName& other) const
{
    //If either is empty, special handling
    if (!isValid() || !other.isValid()) return (this->isValid() == other.isValid());

    //Disable checking if either is incomplete. TODO would this be better in its own function?
    if ((this->flags & Flags::Incomplete) || (this->flags & Flags::Incomplete)) return true;

    if (name.length() != other.name.length()) return false;

    //Compare the stuff that's easy before doing a full string compare
    return nameHash == other.nameHash
        && name == other.name;
}

bool TypeName::operator!=(const TypeName& other) const
{
    //Compare the stuff that's easy before doing a full string compare
    return nameHash != other.nameHash
        || name != other.name;
}

const std::string& TypeName::as_str() const
{
    assert(isValid());
    return name;
}

char const* TypeName::c_str() const
{
    assert(isValid());
    return name.c_str();
}
