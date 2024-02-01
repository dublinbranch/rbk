#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H

#include <boost/describe.hpp>
//if we use this stuff we will have to include boost mp11 later in any case
#include <boost/mp11/algorithm.hpp>

#include "beastConfig.h"

BOOST_DESCRIBE_STRUCT(BeastConf, (), (staticFile, staticFileCacheTTL, logFolder, worker, address, port))

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIGDESCRIBE_H
