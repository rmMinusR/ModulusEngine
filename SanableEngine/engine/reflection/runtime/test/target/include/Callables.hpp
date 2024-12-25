#pragma once

#include "ReflectionSpec.hpp"

namespace Callables
{

	constexpr long magicTestValue = 0xDEADBEEF;

	long __cdecl myFunc1_cdecl();
	void __cdecl myFunc2_cdecl();
	void __cdecl myFunc3_cdecl(int a);
	void __cdecl myFunc4_cdecl(int a, char b);

	long __stdcall myFunc1_stdcall();
	void __stdcall myFunc2_stdcall();
	void __stdcall myFunc3_stdcall(int a);
	void __stdcall myFunc4_stdcall(int a, char b);

	struct STIX_DISABLE_IMAGE_CAPTURE CallableStruct
	{
		long __thiscall myFunc1_thiscall();
		void __thiscall myFunc2_thiscall();
		void __thiscall myFunc3_thiscall(int a);
		void __thiscall myFunc4_thiscall(int a, char b);
	};

}