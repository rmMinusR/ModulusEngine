#include <doctest/doctest.h>

#include "Function.hpp"

struct MyCallable
{
	char padding[64];

	int canary;
	inline MyCallable(int canaryVal = 0) : canary(canaryVal) {}

	inline void incCanary() { ++canary; }
	inline int incCanary2() { return ++canary; }

	inline void setCanary(int val) { canary = val; }
	inline int sum(int a, int b) { return a + b; }


	MyCallable* ptrCanary;
	inline void passStructPtr(MyCallable* v) { ptrCanary =  v; }
	inline void passStructRef(MyCallable& v) { ptrCanary = &v; }
	inline void passStruct   (MyCallable  v) { canary    =  v.canary; }
	inline MyCallable* passthroughStructPtr(MyCallable* v) { return ptrCanary = v; }
	inline MyCallable& passthroughStructRef(MyCallable& v) { return *(ptrCanary = &v); }
	inline MyCallable  passthroughStruct   (MyCallable  v) { canary = v.canary; return v; }
	inline MyCallable* returnStructPtr() { return  ptrCanary; }
	inline MyCallable& returnStructRef() { return *ptrCanary; }
	inline MyCallable  returnStruct   () { return *ptrCanary; }
};

namespace MyCallable_Static //Functions exactly the same as MyCallable, except everything is static
{
	int canary;
	
	inline void incCanary() { ++canary; }
	inline int incCanary2() { return ++canary; }

	inline void setCanary(int val) { canary = val; }
	inline int sum(int a, int b) { return a + b; }


	MyCallable* ptrCanary;
	inline void passStructPtr(MyCallable* v) { ptrCanary =  v; }
	inline void passStructRef(MyCallable& v) { ptrCanary = &v; }
	inline void passStruct   (MyCallable  v) { canary    =  v.canary; }
	inline MyCallable* passthroughStructPtr(MyCallable* v) { return ptrCanary = v; }
	inline MyCallable& passthroughStructRef(MyCallable& v) { return *(ptrCanary = &v); }
	inline MyCallable  passthroughStruct   (MyCallable  v) { canary = v.canary; return v; }
	inline MyCallable* returnStructPtr() { return  ptrCanary; }
	inline MyCallable& returnStructRef() { return *ptrCanary; }
	inline MyCallable  returnStruct   () { return *ptrCanary; }
};

TEST_CASE("Function type erasure")
{
	SUBCASE("Member")
	{
		SUBCASE("Primitive types")
		{
			SUBCASE("void()")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::incCanary);

				MyCallable obj;
				obj.canary = 0;
				fn.invoke(stix::SAnyRef(), stix::SAnyRef::make(&obj), {});

				CHECK(obj.canary == 1);
			}

			SUBCASE("int()")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::incCanary2);

				MyCallable obj;
				obj.canary = 0;
				int result;
				fn.invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), {});

				CHECK(obj.canary == 1);
				CHECK(result == 1);
			}

			SUBCASE("void(int)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::setCanary);

				MyCallable obj;
				obj.canary = 0;
				int val = 20;
				fn.invoke(stix::SAnyRef(), stix::SAnyRef::make(&obj), { stix::SAnyRef::make(&val) });

				CHECK(obj.canary == val);
			}

			SUBCASE("int(int)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::sum);

				MyCallable obj;
				int result;
				int a = 2;
				int b = 3;
				fn.invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&obj), { stix::SAnyRef::make(&a), stix::SAnyRef::make(&b) });

				CHECK(result == a+b);
			}
		}

		SUBCASE("Pointers and references")
		{
			SUBCASE("void(struct*)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passStructPtr);

				MyCallable objA;
				MyCallable objB;
				MyCallable* objB_ptr = &objB;
				fn.invoke(stix::SAnyRef(), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB_ptr) });

				CHECK(objA.ptrCanary == &objB);
			}

			SUBCASE("void(struct&)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passStructRef);

				MyCallable objA;
				MyCallable objB;
				fn.invoke(stix::SAnyRef(), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB) });

				CHECK(objA.ptrCanary == &objB);
			}

			SUBCASE("struct*(struct*)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passthroughStructPtr);

				MyCallable objA;
				MyCallable objB;
				MyCallable* objB_ptr = &objB;
				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB_ptr) });

				CHECK(objA.ptrCanary == &objB);
			}

			SUBCASE("struct&(struct&)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passthroughStructRef);

				MyCallable objA;
				MyCallable objB;
				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB) });

				CHECK(objA.ptrCanary == &objB);
				CHECK(returnVal == &objB);
			}

			SUBCASE("struct*()")
			{
				MyCallable objA;
				MyCallable objB;
				objA.ptrCanary = &objB;

				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::returnStructPtr);

				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), stix::SAnyRef::make(&objA), {});

				CHECK(returnVal == objA.ptrCanary);
			}

			SUBCASE("struct&()")
			{
				MyCallable objA;
				MyCallable objB;
				objA.ptrCanary = &objB;
				objB.canary = 456;

				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::returnStructRef);

				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), stix::SAnyRef::make(&objA), {});

				CHECK(returnVal->canary == objB.canary);
			}
		}

		SUBCASE("Raw structs")
		{
			SUBCASE("void(struct)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passStruct);

				MyCallable objA;
				MyCallable objB;
				objB.canary = 873;
				fn.invoke(stix::SAnyRef(), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB) });

				CHECK(objA.canary == objB.canary);
			}

			SUBCASE("struct(struct)")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::passthroughStruct);

				MyCallable objA;
				MyCallable objB;
				objB.canary = 873;
				MyCallable result;
				fn.invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&objA), { stix::SAnyRef::make(&objB) });

				CHECK(objA.canary == objB.canary);
				CHECK(result.canary == objB.canary);
			}

			SUBCASE("struct()")
			{
				stix::MemberFunction fn = stix::MemberFunction::make(&MyCallable::returnStruct);

				MyCallable objA;
				MyCallable objB;
				objB.canary = 873;
				objA.ptrCanary = &objB;
				MyCallable result;
				fn.invoke(stix::SAnyRef::make(&result), stix::SAnyRef::make(&objA), {});

				CHECK(result.canary == objB.canary);
			}
		}

		/*
		SUBCASE("Const")
		{
			SUBCASE("void(const struct*)") {}
			SUBCASE("void(const struct&)") {}
			SUBCASE("void(const struct)") {}
			SUBCASE("const struct*()") {}
			SUBCASE("const struct&()") {}
			SUBCASE("const struct()") {}
		}
		// */
	}

	
	SUBCASE("Static")
	{
		MyCallable_Static::canary = 0;
		MyCallable_Static::ptrCanary = nullptr;

		SUBCASE("Primitive types")
		{
			SUBCASE("void()")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::incCanary);

				fn.invoke(stix::SAnyRef(), {});

				CHECK(MyCallable_Static::canary == 1);
			}

			SUBCASE("int()")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::incCanary2);

				int result;
				fn.invoke(stix::SAnyRef::make(&result), {});

				CHECK(MyCallable_Static::canary == 1);
				CHECK(result == 1);
			}

			SUBCASE("void(int)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::setCanary);

				int val = 20;
				fn.invoke(stix::SAnyRef(), { stix::SAnyRef::make(&val) });

				CHECK(MyCallable_Static::canary == val);
			}

			SUBCASE("int(int)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::sum);

				int result;
				int a = 2;
				int b = 3;
				fn.invoke(stix::SAnyRef::make(&result), { stix::SAnyRef::make(&a), stix::SAnyRef::make(&b) });

				CHECK(result == a+b);
			}
		}

		SUBCASE("Pointers and references")
		{

			SUBCASE("void(struct*)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passStructPtr);

				MyCallable objB;
				MyCallable* objB_ptr = &objB;
				fn.invoke(stix::SAnyRef(), { stix::SAnyRef::make(&objB_ptr) });

				CHECK(MyCallable_Static::ptrCanary == &objB);
			}

			SUBCASE("void(struct&)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passStructRef);

				MyCallable objB;
				fn.invoke(stix::SAnyRef(), { stix::SAnyRef::make(&objB) });

				CHECK(MyCallable_Static::ptrCanary == &objB);
			}

			SUBCASE("struct*(struct*)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passthroughStructPtr);

				MyCallable objB;
				MyCallable* objB_ptr = &objB;
				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), { stix::SAnyRef::make(&objB_ptr) });

				CHECK(MyCallable_Static::ptrCanary == &objB);
				CHECK(returnVal == &objB);
			}

			SUBCASE("struct&(struct&)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passthroughStructRef);

				MyCallable objB;
				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), { stix::SAnyRef::make(&objB) });

				CHECK(MyCallable_Static::ptrCanary == &objB);
				CHECK(returnVal == &objB);
			}

			SUBCASE("struct*()")
			{
				MyCallable objB;
				MyCallable_Static::ptrCanary = &objB;

				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::returnStructPtr);

				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), {});

				CHECK(returnVal == MyCallable_Static::ptrCanary);
			}

			SUBCASE("struct&()")
			{
				MyCallable objB;
				MyCallable_Static::ptrCanary = &objB;
				objB.canary = 456;

				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::returnStructRef);

				MyCallable* returnVal;
				fn.invoke(stix::SAnyRef::make(&returnVal), {});

				CHECK(returnVal->canary == objB.canary);
			}
		}

		SUBCASE("Raw structs")
		{
			SUBCASE("void(struct)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passStruct);

				MyCallable objB;
				objB.canary = 873;
				fn.invoke(stix::SAnyRef(), { stix::SAnyRef::make(&objB) });

				CHECK(MyCallable_Static::canary == objB.canary);
			}

			SUBCASE("struct(struct)")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::passthroughStruct);

				MyCallable objB;
				objB.canary = 873;
				MyCallable result;
				fn.invoke(stix::SAnyRef::make(&result), { stix::SAnyRef::make(&objB) });

				CHECK(MyCallable_Static::canary == objB.canary);
				CHECK(result.canary == objB.canary);
			}

			SUBCASE("struct()")
			{
				stix::StaticFunction fn = stix::StaticFunction::make(&MyCallable_Static::returnStruct);

				MyCallable objB;
				objB.canary = 873;
				MyCallable_Static::ptrCanary = &objB;
				MyCallable result;
				fn.invoke(stix::SAnyRef::make(&result), {});

				CHECK(result.canary == objB.canary);
			}
		}

		/*
		SUBCASE("Const")
		{
			SUBCASE("void(const struct*)") {}
			SUBCASE("void(const struct&)") {}
			SUBCASE("void(const struct)") {}
			SUBCASE("const struct*()") {}
			SUBCASE("const struct&()") {}
			SUBCASE("const struct()") {}
		}
		// */
	}
}
