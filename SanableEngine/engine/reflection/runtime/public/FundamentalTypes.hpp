#pragma once

#include "dllapi.h"

/*

Reflection data for fundamental types

*/

class ModuleTypeRegistry;
STIX_API void fundamentalTypes_reportRTTI(ModuleTypeRegistry* r);

STIX_API extern const char** fundamentalTypes_names;
STIX_API extern size_t fundamentalTypes_names_sz;


#define FOREACH_FUNDAMENTAL_SIGNED_HELPER(ty) _X(ty) /* _X(signed ty) */ _X(unsigned ty)
#define FOREACH_FUNDAMENTAL_INTEGRAL() \
	FOREACH_FUNDAMENTAL_SIGNED_HELPER(char) \
	_X(wchar_t) /* equivalent to short, but distinct */ \
	FOREACH_FUNDAMENTAL_SIGNED_HELPER(short) /* aka short int */ \
	FOREACH_FUNDAMENTAL_SIGNED_HELPER(int) /* "signed" alone = int */ \
	FOREACH_FUNDAMENTAL_SIGNED_HELPER(long) /* aka long int */ \
	FOREACH_FUNDAMENTAL_SIGNED_HELPER(long long) \

#define FOREACH_FUNDAMENTAL_DECIMAL() \
	_X(float) \
	_X(double) \
	_X(long double)

#define FOREACH_FUNDAMENTAL_TYPE() \
	FOREACH_FUNDAMENTAL_INTEGRAL() \
	FOREACH_FUNDAMENTAL_DECIMAL()
