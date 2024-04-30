#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H

#include "rbk/misc/intTypes.h"
#include <string>

void requestBeging();
void requestEnd();
i64  registerFlushTime();

std::string composeStatus();

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H
