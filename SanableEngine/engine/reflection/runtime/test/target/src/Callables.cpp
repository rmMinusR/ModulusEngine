#include "Callables.hpp"

long __cdecl Callables::myFunc1_cdecl() { return magicTestValue; }
void __cdecl Callables::myFunc2_cdecl() {}
void __cdecl Callables::myFunc3_cdecl(int a) {}
void __cdecl Callables::myFunc4_cdecl(int a, char b) {}

long __stdcall Callables::myFunc1_stdcall() { return magicTestValue; }
void __stdcall Callables::myFunc2_stdcall() {}
void __stdcall Callables::myFunc3_stdcall(int a) {}
void __stdcall Callables::myFunc4_stdcall(int a, char b) {}

long __thiscall Callables::CallableStruct::myFunc1_thiscall() { return magicTestValue; }
void __thiscall Callables::CallableStruct::myFunc2_thiscall() {}
void __thiscall Callables::CallableStruct::myFunc3_thiscall(int a) {}
void __thiscall Callables::CallableStruct::myFunc4_thiscall(int a, char b) {}
