//absolute instant compilation time to provide fresh info
#include "buffer.h"
#if defined(__has_include)
#	if __has_include("git_status_generated.h")
#		include "git_status_generated.h"
#	endif
#endif

void loadBuffer() {
	COMPILATION_TIME_buffer = __DATE__ " " __TIME__;
#ifdef GIT_STATUS
	GIT_STATUS_buffer = GIT_STATUS;
#else
	GIT_STATUS_buffer = "Not Available";
#endif
	//GIT_SUBMODULES_buffer   = GIT_SUBMODULES;
}
