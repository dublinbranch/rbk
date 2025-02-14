//absolute instant compilation time to provide fresh info
#include "buffer.h"
#include <QDateTime>
#include <rbk/defines/stringDefine.h>

void loadBuffer() {
	COMPILATION_TIME_buffer = __DATE__ " " __TIME__;
#ifdef GIT_STATUS
	GIT_STATUS_buffer = GIT_STATUS;
#else
	GIT_STATUS_buffer = "Not Available";
#endif
	//GIT_SUBMODULES_buffer   = GIT_SUBMODULES;
}
