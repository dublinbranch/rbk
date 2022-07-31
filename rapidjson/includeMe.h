#ifndef RAPIDJSON_INCLUDEME_H
#define RAPIDJSON_INCLUDEME_H

//Do not call ABORT, call our custom assert which has inside the stack unwid via Backward
#define RAPIDJSON_ASSERT(x) rapidAssert(x);
void rapidAssert(bool condition);

//Choose to throw on error or not https://github.com/Tencent/rapidjson/issues/1606
inline thread_local bool rapidAssertEnabled    = true;
inline thread_local bool rapidAssertPrintTrace = true;

//this will require QMAKE_CXXFLAGS += -msse4.2
#define RAPIDJSON_SSE42 1 //why not ? we do not target gameboy anymore

#define RAPIDJSON_48BITPOINTER_OPTIMIZATION 1 //stuff some data in the high part of 64bit ptr (x64 IGNORE after bit 48)
#define RAPIDJSON_PARSE_DEFAULT_FLAGS 32      //add the ability to have comment inside

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

typedef rapidjson::GenericValue<rapidjson::UTF8<>> jsonValue;

#endif // RAPIDJSON_INCLUDEME_H
