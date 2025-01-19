#pragma once

namespace Overriding
{

	class VirtFuncBase
	{
	public:
		virtual void foo();
	};

	class AbstrFuncBase
	{
	public:
		virtual void foo() = 0;
	};

	
	// Funcs: [virtual] void foo() [override] [= 0];

#define DECL_CASE_VIRT_BASE(Name, decl) class Name : public VirtFuncBase { public: decl; }

	DECL_CASE_VIRT_BASE(VirtBaseFunc_ExplicitVirt_ExplicitOverride_Concrete, virtual void foo() override);
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ExplicitVirt_ImplicitOverride_Concrete, virtual void foo());
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ImplicitVirt_ExplicitOverride_Concrete, void foo() override);
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ImplicitVirt_ImplicitOverride_Concrete, void foo());
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ExplicitVirt_ExplicitOverride_Abstracted, virtual void foo() override = 0);
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ExplicitVirt_ImplicitOverride_Abstracted, virtual void foo() = 0);
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ImplicitVirt_ExplicitOverride_Abstracted, void foo() override = 0);
	DECL_CASE_VIRT_BASE(VirtBaseFunc_ImplicitVirt_ImplicitOverride_Abstracted, void foo() = 0);

	// Abstract funcs: [virtual] void foo() [override] = 0;

#define DECL_CASE_ABSTR_BASE(Name, decl) class Name : public AbstrFuncBase { public: decl; }

	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ExplicitVirt_ExplicitOverride_Concrete, virtual void foo() override);
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ExplicitVirt_ImplicitOverride_Concrete, virtual void foo());
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ImplicitVirt_ExplicitOverride_Concrete, void foo() override);
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ImplicitVirt_ImplicitOverride_Concrete, void foo());
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ExplicitVirt_ExplicitOverride_Abstracted, virtual void foo() override = 0);
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ExplicitVirt_ImplicitOverride_Abstracted, virtual void foo() = 0);
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ImplicitVirt_ExplicitOverride_Abstracted, void foo() override = 0);
	DECL_CASE_ABSTR_BASE(AbstrBaseFunc_ImplicitVirt_ImplicitOverride_Abstracted, void foo() = 0);


	// Ctor

	class VirtDtorBase
	{
	public:
		virtual ~VirtDtorBase();
	};

	class ImplicitVirtDtor : public VirtDtorBase {};

	class ExplicitVirtDtor : public VirtDtorBase
	{
	public:
		virtual ~ExplicitVirtDtor();
	};

	class ExplicitDtorImplicitlyVirt : public VirtDtorBase
	{
	public:
		~ExplicitDtorImplicitlyVirt();
	};

}
