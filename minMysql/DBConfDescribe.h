#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H

#include "DBConf.h"
#include <boost/describe.hpp>

//the full one maybe contain too many info not really usefull in this case
//BOOST_DESCRIBE_STRUCT(DBConf, (), (host, pass, user, sock, cacheId, ssl, writeBinlog, isMariaDB8, warningSuppression, readTimeout, port, logSql, logError, pingBeforeQuery, compress, NULL_as_EMPTY, connErrorVerbosity, defaultDB))
BOOST_DESCRIBE_STRUCT(DBConf, (), (host, pass, user, sock, ssl, port, readTimeout, logSql, logError, defaultDB, isMariaDB8))

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H
