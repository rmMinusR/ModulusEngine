#include "VirtualInheritance.hpp"

#include <typeinfo>

std::string VirtualSharedBase::identify() const { return typeid(decltype(*this)).name(); }
std::string VirtualInheritedA::identify() const { return typeid(decltype(*this)).name(); }
std::string VirtualInheritedB::identify() const { return typeid(decltype(*this)).name(); }
std::string VirtualDiamond   ::identify() const { return typeid(decltype(*this)).name(); }

std::string VirtualSharedBase::identify_s() { return typeid(VirtualSharedBase).name(); }
std::string VirtualInheritedA::identify_s() { return typeid(VirtualInheritedA).name(); }
std::string VirtualInheritedB::identify_s() { return typeid(VirtualInheritedB).name(); }
std::string VirtualDiamond   ::identify_s() { return typeid(VirtualDiamond   ).name(); }
