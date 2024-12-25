#pragma once

#include <string>

struct VirtualSharedBase
{
	virtual ~VirtualSharedBase() = default;
	virtual std::string identify() const;
	static std::string identify_s();
};

struct VirtualInheritedA : public virtual VirtualSharedBase
{
	virtual ~VirtualInheritedA() = default;
	virtual std::string identify() const;
	static std::string identify_s();
};

struct VirtualInheritedB : public virtual VirtualSharedBase
{
	virtual ~VirtualInheritedB() = default;
	virtual std::string identify() const;
	static std::string identify_s();
};

struct VirtualDiamond : public virtual VirtualInheritedA, public VirtualInheritedB
{
	virtual ~VirtualDiamond() = default;
	virtual std::string identify() const;
	static std::string identify_s();
};
