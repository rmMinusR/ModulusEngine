#pragma once

//Basic existance checks: types
template<typename T> struct TemplatedType_Typename { T*    memFn() { return nullptr; } };
template<class    T> struct TemplatedType_Class    { T*    memFn() { return nullptr; } };
template<int    val> struct TemplatedType_Int      { int   memFn() { return val    ; } };
template<void*  val> struct TemplatedType_VoidPtr  { void* memFn() { return val    ; } };

//Existance checks: defaulted template parameters
template<typename T = int> struct TemplatedType_DefaultType {};
template<int val    = 3  > struct TemplatedType_DefaultInt {};

//Curveball: nameless parameters
template<typename> struct TemplatedType_NamelessParam {};
template<typename = int> struct TemplatedType_NamelessDefaultedParam {};

//Basic existance checks: functions
//TODO
//template<typename T> T&    TemplatedGlobFn_Typename(T& val) { return val; }
//template<class    T> T&    TemplatedGlobFn_Class   (T& val) { return val; }
//template<int    val> int   TemplatedGlobFn_Int     () { return val; }
//template<void*  val> void* TemplatedGlobFn_VoidPtr () { return val; }


template<typename T>
struct TemplatedStaticsContainer
{
    static const int val;
};

template<typename T> const int TemplatedStaticsContainer<T>::val = 2;
