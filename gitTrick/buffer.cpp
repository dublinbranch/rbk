//absolute instant compilation time to provide fresh info
#include "buffer.h"
//#include <string>

//constexpr std::string GIT_SHA1 = []() constexpr {
//	return "GIT_SHA1_";
//}();

//how to constinit concat 2 string ? I will just keep using preprocessor-.-

#define GIT_SHA "GIT_SHA1_" GIT_STATUS;

void loadBuffer() {
	//if the macro is not used looks like is discarded
	auto x = GIT_SHA;
	//so it does not complain is not used
	(void)x;
	COMPILATION_TIME_buffer = COMPILATION_TIME;
	GIT_STATUS_buffer       = GIT_STATUS;
	//GIT_SUBMODULES_buffer   = GIT_SUBMODULES;
}
