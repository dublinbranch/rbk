#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H

#include "rbk/misc/intTypes.h"
#include "statusPageConf.h"
#include <string>

void requestBeging();
void requestEnd();
i64  registerFlushTime();

// Set once from the config load, before the HttpHandler threads start
void                  setStatusPageConf(const StatusPageConf& c);
const StatusPageConf& statusPageConf();

std::string composeStatus();
// The TCP socket page, see StatusPageConf::sockets
std::string composeSocketsStatus();

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_THREAD_TMONITORING_H
