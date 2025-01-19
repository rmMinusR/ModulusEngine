//Deliberately no include guards

#if _WIN32
#define API_EXPORT __declspec(dllexport)
#define API_IMPORT __declspec(dllimport)
#define API_KEEPALIVE
#elif __EMSCRIPTEN__
#include <emscripten.h>
#define API_EXPORT
#define API_IMPORT
#define API_KEEPALIVE EMSCRIPTEN_KEEPALIVE
#else
#error Unknown platform: don't know how to export shared library symbols
#endif

#if stix_EXPORTS
#define STIX_API API_EXPORT API_KEEPALIVE
#define STIX_INTERNAL(def) def
#else
#define STIX_API API_IMPORT API_KEEPALIVE
#define STIX_INTERNAL(def)
#endif
