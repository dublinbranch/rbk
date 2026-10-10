#ifndef HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONFDESCRIBE_H
#define HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONFDESCRIBE_H

#include "statusPageConf.h"
#include <boost/describe.hpp>

BOOST_DESCRIBE_STRUCT(StatusPageConf, (), (process, threads, fdKinds, mysqlPing, host, diskPaths, network, unitNet, sockets, socketTop))

#endif // HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONFDESCRIBE_H
