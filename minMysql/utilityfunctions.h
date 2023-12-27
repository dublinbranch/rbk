#ifndef MIN_MYSQL_UTILITYFUNCTIONS_H
#define MIN_MYSQL_UTILITYFUNCTIONS_H

#include "min_mysql.h"
#include "rbk/BoostJson/fwd.h"

class DBDebugger : public DB {
      public:
	DBDebugger() = default;
	DBDebugger(const DBConf& _conf);

	sqlResult getRunningQueries();
	sqlResult getProcessList();
};

sqlResult filterRunningQueries(const sqlResult& sqlProcessList);
//something like a partial result ? no clear idea what is supposed to do
QString queryEssay(const sqlRow& row, bool brief);
QString queryEssay(const sqlResult& res, bool brief, bool skipNull = false);

std::vector<std::string> getTablesInDB(DB* db, std::string_view schema);

boost::json::object row2json(const sqlRow& row);
boost::json::object res2json(const sqlResult& row);
#endif // UTILITYFUNCTIONS_H
