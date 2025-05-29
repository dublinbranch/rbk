#include "utilityfunctions.h"
#include "QtDebug"
#include "rbk/fmtExtra/includeMe.h"
#include <boost/json/object.hpp>

using namespace std;

sqlResult filterRunningQueries(const sqlResult& sqlProcessList) {
	sqlResult res;
	for (auto& sql : sqlProcessList) {
		auto command = sql.get("Command");
		if ((command == QBL("Query")) or (command == QBL("Connect"))) {
			res.push_back(sql);
		}
	}
	return res;
}

QString queryEssay(const sqlRow& row, bool brief) {
	QString time     = row["Time"];
	QString fullInfo = row["Info"];

	QString info;
	if (brief) {
		//looks as a reasonable lenght to avoid cluttering and mostly understand was is going on
		info = fullInfo.left(256);
	} else {
		info = fullInfo;
	}

	return QSL("for %1 s : %2 ")
	           .arg(time, info) +
	       "\n";
}

QString queryEssay(const sqlResult& res, bool brief, bool skipNull) {
	QString msg;
	for (auto&& query : res) {
		auto info = query.get2<QString>("Info");
		if (skipNull and (info == "NULL")) {
			continue;
		}
		msg.append(queryEssay(query, brief));
	}
	return msg;
}

DBDebugger::DBDebugger(const DBConf& _conf) {
	setConf(_conf);
}

sqlResult DBDebugger::getRunningQueries() {
	return filterRunningQueries(getProcessList());
}

sqlResult DBDebugger::getProcessList() {
	auto sql = "show full processlist";
	auto res = query(sql);
	return res;
}

std::vector<std::string> getTablesInDB(DB* db, std::string_view schema) {
	auto s   = db->escape(schema);
	auto sql = F(R"(
SELECT TABLE_NAME 
FROM information_schema.`TABLES` 
WHERE `TABLE_SCHEMA` = '{}' 
	AND `TABLE_TYPE` = 'BASE TABLE'
ORDER BY CREATE_TIME DESC)",
	             schema);
	auto res = db->query(sql);

	std::vector<std::string> f;

	for (auto& row : res) {
		f.push_back(row.rq<std::string>("TABLE_NAME"));
	}
	return f;
}

boost::json::object row2json(const sqlRow& row) {
	boost::json::object obj;
	for (auto&& [k, v] : row) {
		//at the moment we ignore the type of the value
		obj[k.toStdString()] = v.toStdString();
	}
	return obj;
}

bj::object res2json(const sqlResult& res) {
	bj::object final;
	final["fromCache"] = res.fromCache;
	bj::array arr;
	for (auto& row : res) {
		arr.push_back(row2json(row));
	}
	final["rows"] = arr;
	return final;
}

bool tableExists(string_view db, string_view table, DB* conn) {
	//Check the target table do not exists
	string sql = F(R"(
SELECT COUNT(*) AS tbl_exists
FROM information_schema.tables
WHERE table_schema = '{}'
  AND table_name   = '{}';
)",
	               db, table);

	auto row = conn->queryCacheLineV2(sql, 0, true);
	return row.rq<int>("tbl_exists");
}

bool swapTable(string_view sourceDb, string_view sourceTable,
               string_view targetDb, string_view targetTable, DB* conn) {
	auto tmp = F("{}_{}", targetTable, QDateTime::currentDateTime().toString(fileDateTimeFormat));
	auto sql = F("RENAME TABLE `{}`.`{}` TO `{}`.`{}`, `{}`.`{}` To `{}`.`{}`;",
	             targetDb, targetTable,
	             targetDb, tmp,
	             sourceDb, sourceTable,
	             targetDb, targetTable);
	conn->query(sql);
	return true;
}
