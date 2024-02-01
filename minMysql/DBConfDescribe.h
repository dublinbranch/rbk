#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H

#include <boost/describe.hpp>
//if we use this stuff we will have to include boost mp11 later in any case
#include <boost/mp11/algorithm.hpp>

#include "boost/json/fwd.hpp"

#include "DBConf.h"

//the full one maybe contain too many info not really usefull in this case
//BOOST_DESCRIBE_STRUCT(DBConf, (), (host, pass, user, sock, cacheId, ssl, writeBinlog, isMariaDB8, warningSuppression, readTimeout, port, logSql, logError, pingBeforeQuery, compress, NULL_as_EMPTY, connErrorVerbosity, defaultDB))
BOOST_DESCRIBE_STRUCT(DBConf, (), (host, pass, user, sock, ssl, port, readTimeout, logSql, logError, defaultDB, isMariaDB8))

namespace boost {
namespace json {
template <>
struct is_described_class<DBConf> : std::true_type {};
} // namespace json
} // namespace boost
#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MINMYSQL_DBCONFDESCRIBE_H
