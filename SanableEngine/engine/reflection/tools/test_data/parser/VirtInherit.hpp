#pragma once

class DiamondSharedBase
{
	int foo;
	virtual ~DiamondSharedBase() = default;
};

class DiamondA : public virtual DiamondSharedBase {};
class DiamondB : public virtual DiamondSharedBase {};
class DiamondGrandchild : public virtual DiamondA, public virtual DiamondB {};
