#ifndef RAPIDJSON_INCLUDEME_H
#define RAPIDJSON_INCLUDEME_H

//Do not call ABORT, call our custom assert which has inside the stack unwid via Backward
#define RAPIDJSON_ASSERT(x) rapidAssert(x);
void rapidAssert(bool condition);

//Choose to throw on error or not https://github.com/Tencent/rapidjson/issues/1606
inline thread_local bool rapidAssertEnabled    = true;
inline thread_local bool rapidAssertPrintTrace = true;


//check if we are in X64
#if defined(__x86_64__) || defined(_M_X64)
#define RAPIDJSON_SSE42 1
#elif defined(__ARM_NEON) || defined(_M_ARM_NEON)
#define RAPIDJSON_NEON 1
#endif

#define RAPIDJSON_48BITPOINTER_OPTIMIZATION 1 //stuff some data in the high part of 64bit ptr (x64 IGNORE after bit 48)
#define RAPIDJSON_PARSE_DEFAULT_FLAGS 32      //add the ability to have comment inside

#pragma GCC diagnostic push

//internally they use i64 wich is a well know alias
#pragma GCC diagnostic ignored "-Wshadow"

#include "rbk/rapidjson/document.h"
#include "rbk/rapidjson/stringbuffer.h"
#include "rbk/rapidjson/writer.h"

#pragma GCC diagnostic pop

typedef rapidjson::GenericValue<rapidjson::UTF8<>> jsonValue;

#endif // RAPIDJSON_INCLUDEME_H
