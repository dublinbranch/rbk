#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H

#include "beastConfig.h"
#include <boost/describe.hpp>

BOOST_DESCRIBE_STRUCT(BeastConf, (), (staticFile, staticFileCacheTTL, htmlAllException, logFolder, worker, address, port, logRequest, maxResponseSize, logResponse, logRequestByIp, logWhitelist, logBlacklist, basePath))

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H
