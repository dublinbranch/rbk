#include "min_mysql.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII//resetAfterUse.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/hash/sha.h"
#include "rbk/misc/b64.h"
#include "rbk/serialization/QDataStreamer.h"
#include "rbk/serialization/serialize.h"
#include "rbk/thread/threadstatush.h"
#include "utilityfunctions.h"
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <memory>
#include <mutex>
#include <qscopeguard.h>
#include <unistd.h>

extern thread_local ThreadStatus::Status* localThreadStatus;

//TODO
/*
 * definisco qui il runnable globale piuttosto che nel progetto main
 * motivo:
 * se questo runnable fosse settato nel progetto main e mi serivisse anche qui come propagarlo?
 * non posso fare copia globale in questo modulo r2 = runnable perché non copiabile
 * posso fare puntare r3 = &runnable ma poi scomodo da usare costretto fare: r3->runnable("key",1)...
 * quindi meglio definire qui il runnable globale
 */

/*
 * to use, in main project:
 * extern Runnable runnable;
 * runnable.setConf(conf);
 *
 * must be set also for functions that use it (e.g. ClickHouse::query())
 */
//Runnable runnable;

DB::SharedState DB::sharedState;
using namespace std;

static int somethingHappened(MYSQL* mysql, int status);

sqlRow DB::queryLine(const StringAdt& sql) const {
	auto res = query(sql);
	if (res.empty()) {
		return {};
	}
	return res[0];
}

void DB::setMaxQueryTime(uint time) const {
	query(QSL("SET @@max_statement_time  = %1").arg(time));
}

sqlResult DB::query(const StringAdt& sql) const {
	if (sql.empty()) {
		return {};
	}

	auto logger = queryInner(sql);

	if (noFetch) {
		return {};
	}
	return fetchResult(&logger);
}

SQLLogger DB::queryInner(const std::string& sql) const {
	ResetOnExit reset1(localThreadStatus->state, ThreadState::MyQuery);
	localThreadStatus->sql = sql;

	auto conn = getConn();
	if (conn == nullptr) {
		throw ExceptionV2("This mysql instance is not connected!");
	}

	SQLLogger sqlLogger(sql, conf.logError.value(), this);
	if (sql != "SHOW WARNINGS") {
		state.get().lastSQL = sql;
		sqlLogger.logSql    = conf.logSql.value();
	} else {
		sqlLogger.logSql = false;
	}

	pingCheck(conn);

	{
		QElapsedTimer timer;
		timer.start();

		sharedState.busyConnection++;
		mysql_query(conn, sql.c_str());
		sharedState.busyConnection--;
		sqlLogger.serverTime = timer.nsecsElapsed();
		auto& st             = state.get();
		st.queryExecuted++;
		st.serverTime = sqlLogger.serverTime;
		st.totServerTime += sqlLogger.serverTime;
	}

	auto error           = mysql_errno(conn);
	state->lastErrorCode = error;
	if (error) {
		switch (error) {
		case 1062: { //unique index violation
			if (state->uniqueViolationNothrow) {
				state->lastError = mysql_error(conn);
				return sqlLogger;
			} else {
				cxaNoStack     = true;
				cxaLevel       = CxaLevel::none;
				auto msg       = F("After: {} \n {} \nFor:\n{}", sqlLogger.serverTime, mysql_error(conn), sql);
				auto exception = DBException(msg, DBException::Error(error));
				throw exception;
			}
		}
		case 1065:
			// well an empty query is bad, but not too much!
			qWarning().noquote() << F16("empty query (or equivalent for) {} in {}", sql, stacker());
			return sqlLogger;

		// Lock wait timeout exceeded; try restarting transaction
		case 1205:
		// Deadlock found when trying to get lock; try restarting transaction
		case 1213:
		// Lost connection to MySQL server during query
		case 2013: {
			string err;

			if (state.get().skipNextDisconnect) {
				state.get().skipNextDisconnect = false;
			} else {
				// This is sometimes happening, and I really have no idea how to fix, there is already the ping at the beginning, but looks like is not working...
				// so we try to get some info

				static const auto errSkel = R"(Mysql error for {}
error was {} code: {}, connInfo: {}
thread: {}, queryDone: {}, reconnection: {}, busyConn: {}, totConn: {}, queryTime: {:.3f} nanoseconds ({} nanoseconds)
{})";

				// if error is deadlock then list all running sql's
				QString runningSqls;
				if ((error == 1213) or (error == 1205)) {
					DB   db2;
					auto conf2 = getConf();
					db2.setConf(conf2);

					auto sqlInfo = QSL("SHOW FULL PROCESSLIST");
					auto res     = db2.query(sqlInfo);
					runningSqls  = QSL("running sql's:\n%1").arg(queryEssay(res, false, true));
				}

				err = F(errSkel,
				        sql,
				        mysql_error(conn),
				        error,
				        conf.getInfo(),
				        mysql_thread_id(conn),
				        state.get().queryExecuted,
				        state.get().reconnection,
				        sharedState.busyConnection.load(),
				        St_mysqlW::connCounter.load(),
				        static_cast<double>(sqlLogger.serverTime),
				        sqlLogger.serverTime,
				        runningSqls);

				sqlLogger.error = err;

				qWarning().noquote() << QString::fromStdString(err) << QStacker16();
			}

			closeConn();

			cxaNoStack     = true;
			cxaLevel       = CxaLevel::none;
			auto exception = DBException(err, DBException::Error(error));
			throw exception;
		} break;
		default:
			string err = F(R"(Mysql error for sql:
{}
Error: {} 
Code: {}
Connection Info: {})",
			             sql, mysql_error(conn), error, getConf().getInfo());

			sqlLogger.error = err;
			// this line is needed for proper email error reporting
			{
				//many times this call is very nested, so we bump the default stack trace lenght
				ResetOnExit r(stackerMaxFrame, (uint)25);
				qWarning().noquote() << QString::fromStdString(err) << QStacker16();
				cxaNoStack     = false;
				auto exception = DBException(err, DBException::Error::NA);
				throw exception;
			}
		}
	}
	return sqlLogger;
}

SqlRowV2 DB::queryCacheLineV2(const StringAdt& sql, uint ttl, bool required) {
	auto res = queryCacheV2(sql, ttl);
	if (auto r = res.size(); r > 1) {
		auto msg = F("invalid number of row for: {}\nExpected 1, got {} \n", sql, r);
		throw ExceptionV2(msg);
	} else if (r == 1) {
		return *res.begin();
	} else {
		if (required) {
			throw DBException("no result for " + sql, DBException::NoResult);
		} else {
			return {};
		}
	}
}

sqlRow DB::queryCacheLine2(const StringAdt& sql, uint ttl, bool required) {
	//TODO forward the check into query cache line ?
	auto res = queryCache2(sql, ttl);
	if (auto r = res.size(); r > 1) {
		auto msg = F("invalid number of row for: {}\nExpected 1, got {} \n", sql, r);
		throw ExceptionV2(msg);
	} else if (r == 1) {
		auto& row = res[0];
		return row;
	} else {
		if (required) {
			throw DBException("no result for " + sql, DBException::NoResult);
		} else {
			return {};
		}
	}
}

sqlResult DB::queryCache2(const StringAdt& sql, const Opt& opt) const {
	state.get().noCacheOnEmpty = opt.noCacheOnEmpty;
	return queryCache2(QString::fromStdString(sql), opt.ttl, opt.required);
}

//To force load from cache use something like
//	DBConf c;
//	c.host            = "127.0.0.1";
//	c.port            = 3306;
//	c.user            = "roy";
//	c.pass            = "roy";
//	c.pingBeforeQuery = true;
//	c.logError        = true;
//	c.logSql          = true;
//	c.setDefaultDB("test");

//	DB r(c);

//	QString   sql = "select now(6)";
//	sqlResult res;
//	while (true) {
//		try {
//			res = r.queryCache2(sql, 1);
//			qDebug() << res;
//		} catch (DBException& e) {
//			//In case they are offline we just use old data
//			if (e.errorType == DBException::Connection) {
//				res = r.queryCache2(sql, numeric_limits<uint>::max(), true);
//			} else {
//				throw;
//			}
//		}
//	}

sqlResult DB::queryCache2(const StringAdt& sql, uint ttl, bool required) const {
	ResetOnExit<typeof localThreadStatus->state> reset1;
	if (localThreadStatus) {
		reset1.set(localThreadStatus->state, ThreadState::MyCache);
	}

	SetOnExit noCache(state.get().noCacheOnEmpty, false);
	if (ttl) {
		// small trick to avoid calling over and over the function
		static bool fraud = mkdir(QSL("cachedSQL_%1/").arg(conf.cacheId));
		(void)fraud;

		//We have a lock to prevent concurrent write in this process, but nothing to protect against other, just use another folder
		QString name = QSL("cachedSQL_%1/").arg(conf.cacheId) + sha1(sql);

		//We do not really care about cache stampede here... and we write the file in an atomic way so we avoid torn read. So no need for mutex,
		//if multiple thread will write the same file is not a problem they will just overwrite the final version and not mangle each other
		sqlResult res;
		{
			localThreadStatus->time.IO.start();
			OnExit ex([]() { localThreadStatus->time.IO.pause(); });
			if (auto file = fileUnSerialize(name, res, ttl); file.valid) {
				for (auto& row : res) {
					row.fromCache = true;
				}
				res.fromCache = true;
				return res;
			}
		}
		//FIXME not very clear, if I pass an extremely long ttl I must load from cache ?
		if (ttl == numeric_limits<typeof(ttl)>::max()) {
			if (required) {
				throw DBException("no result for " + sql, DBException::NoResult);
			} else {
				return sqlResult();
			}
		}

		res = query(sql);

		//TODO add some check to serialize ONLY if the result is ok
		if (res.empty()) {
			if (state.get().noCacheOnEmpty) {
				return res;
			}
		}
		fileSerialize(name, res);
		return res;
	} else {
		return query(sql);
	}
}

SqlResultV2 DB::queryCacheV2(const StringAdt& sql, uint ttl) {
	ResetOnExit<typeof localThreadStatus->state> reset1;
	if (localThreadStatus) {
		reset1.set(localThreadStatus->state, ThreadState::MyCache);
	}

	SetOnExit noCache(state.get().noCacheOnEmpty, false);
	if (ttl) {
		// small trick to avoid calling over and over the function
		static bool fraud = mkdir(QSL("cachedSQL_V2_%1/").arg(conf.cacheId));
		(void)fraud;

		//We have a lock to prevent concurrent write in this process, but nothing to protect against other, just use another folder
		QString name = QSL("cachedSQL_V2_%1/").arg(conf.cacheId) + sha1(sql);

		//We do not really care about cache stampede here... and we write the file in an atomic way so we avoid torn read. So no need for mutex,
		//if multiple thread will write the same file is not a problem they will just overwrite the final version and not mangle each other
		SqlResultV2 res;
		{
			localThreadStatus->time.IO.start();
			OnExit ex([]() { localThreadStatus->time.IO.pause(); });
			if (auto file = fileUnSerialize(name, res, ttl); file.valid) {
				return res;
			}
		}

		res = queryV2(sql);

		//TODO add some check to serialize ONLY if the result is ok
		if (res.empty()) {
			if (state.get().noCacheOnEmpty) {
				return res;
			}
		}
		fileSerialize(name, res);
		return res;
	} else {
		return queryV2(sql);
	}
}

sqlResult DB::queryORcache(const StringAdt& sql, uint ttl, bool required) {
	try {
		return queryCache2(sql, ttl, false);
	} catch (DBException& e) {
		if (e.errorType == DBException::Connection) {
			//Duplicated code, but I have no idea how to avoid that without rigmarole

			QString                      name = QSL("cachedSQL_%1/").arg(conf.cacheId) + sha1(sql);
			static std::mutex            lock;
			std::scoped_lock<std::mutex> scoped(lock);

			sqlResult res;
			if (auto file = fileUnSerialize(name, res, numeric_limits<typeof(ttl)>::max()); file.valid) {
				for (auto& row : res) {
					row.fromCache = true;
				}
				res.fromCache = true;
				return res;
			} else if (required) {
				throw DBException("NO result and NO cache for " + sql + " Original sql excpetion is " + e.what(), DBException::NoResult);
			}
			return sqlResult();
		}
	}
	//This will rethrow the same exception
	throw;
}

sqlResult DB::queryDeadlockRepeater(const QByteArray& sql, uint maxTry) const {
	sqlResult result;
	if (!sql.isEmpty()) {
		for (uint tryNum = 0; tryNum < maxTry; ++tryNum) {
			try {
				return query(sql);
			} catch (const DBException& error) {
				switch (error.errorType) {
				case DBException::Error::DeadLock:
					continue;
					break;
				default:
					throw;
				}
			} catch (...) {
				throw;
			}
		}
		qWarning().noquote() << "too many trial to resolve deadlock, fix your code!" + QStacker16();
		cxaNoStack = true;
		throw DBException("Deadlock for " + sql, DBException::DeadLock);
	}
	return result;
}

void DB::pingCheck(st_mysql*& conn) const {
	// can be disabled in local host to run a bit faster on laggy connection
	if (!conf.pingBeforeQuery.value_or(true)) {
		return;
	}
	SQLLogger sqlLogger("PING", conf.logError.value(), this);
	auto      oldConnId = mysql_thread_id(conn);

	auto guard = qScopeGuard([&] {
		auto newConnId = mysql_thread_id(conn);
		if (oldConnId != newConnId) {
			state.get().reconnection++;
			qDebug() << "detected mysql reconnection";
		}
	});

	int connRetry = 0;
	// Those will not emit an error, only the last one
	for (; connRetry < 5; connRetry++) {
		if (mysql_ping(conn)) { // 1 on error, which should not even happen ... but here we are
			// force reconnection
			closeConn();
			conn = getConn();
		} else {
			return;
		}
	}
	// last ping check
	if (mysql_ping(conn)) { // 1 on error
		auto error = mysql_errno(conn);
		auto err   = F("Mysql error for {} \nerror was {} code: {}, connRetry for {}, connectionId: {}, conf: ", sqlLogger.sql, mysql_error(conn), error, connRetry, mysql_error(conn)) +
		           conf.getInfo() +
		           stacker();
		sqlLogger.error = err;
		// this line is needed for proper email error reporting
		qWarning().noquote() << QString::fromStdString(err);
		cxaNoStack = true;
		throw err;
	}

	//no ned write anything if is working
	sqlLogger.logSql = false;
	return;
}

QString DB::escape(const QString& what) const {
	auto plain = what.toUtf8();
	// Ma esiste una lib in C++ per mysql ?
	char* tStr = new char[(uint)plain.size() * 2 + 1];
	mysql_real_escape_string(getConn(), tStr, plain.constData(), (u64)plain.size());
	auto escaped = QString::fromUtf8(tStr);
	delete[] tStr;
	return escaped;
}

string DB::escape(const std::string& plain) const {
	return escape(std::string_view(plain));
}

string DB::escape(const std::string_view plain) const {
	// Ma esiste una lib in C++ per mysql ?
	char* tStr = new char[plain.size() * 2 + 1];
	mysql_real_escape_string(getConn(), tStr, plain.data(), plain.size());
	string escaped = tStr;
	delete[] tStr;
	return escaped;
}

QString QV(const sqlRow& line, const QByteArray& b) {
	return line.value(b);
}

st_mysql* DB::getConn(bool doNotConnect) const {
	auto curConn = connPool.get();
	if (curConn == nullptr) {
		if (doNotConnect) {
			return nullptr;
		}
		connect();
	}
	return *connPool.get();
}

u64 DB::lastId() const {
	auto lastId = mysql_insert_id(getConn());
	if (!lastId) {
		throw DBException("Last insert is 0, check for error!", DBException::InvalidState);
	}
	return lastId;
}

u64 DB::lastIdNoEx() const {
	return mysql_insert_id(getConn());
}

const DBConf DB::getConf() const {
	if (!confSet) {
		cxaNoStack = true;
		throw ExceptionV2("you have not set the configuration!");
	}
	return conf;
}

void DB::setConfIfNotSet(const DBConf& value) {
	if (!confSet) {
		setConf(value);
	}
}

void DB::setConf(const DBConf& value) {
	if (confSet) {
		throw ExceptionV2("better not set twice the db config to avoid abomination");
	}
	conf    = value;
	confSet = true;
	for (auto& rx : conf.warningSuppression) {
		rx->optimize();
	}
	state.get().NULL_as_EMPTY = conf.NULL_as_EMPTY;
}

long DB::getAffectedRows() const {
	return affectedRows;
}

DBConf::DBConf() {
}

QByteArray DBConf::getDefaultDB() const {
	if (defaultDB.isEmpty()) {

		auto msg = F16(R"(
default DB is sadly required to avoid mysql complain on certain operation!"
current config is 
user: {} 
host: {}
stack: {}
)",
		               user, host, QStacker16Light());
		qWarning().noquote() << msg;
		cxaNoStack = true;
		throw ExceptionV2(msg);
	}
	return defaultDB;
}

void DBConf::setDefaultDB(const QByteArray& value) {
	defaultDB = value;
}

std::string DBConf::getInfo(bool passwd) const {
	auto msg = F(" {}:{}  user: {}", host, port.value(), user.data());
	if (passwd) {
		msg += pass.data();
	}
	return msg;
}

void DBConf::setWarningSuppression(std::vector<QString> regexes) {
	for (auto& str : regexes) {
		warningSuppression.push_back(make_shared<QRegularExpression>(str));
	}
}

DB::DB(const DBConf& _conf) {
	setConf(_conf);
}

DB::~DB() {
	// will be later removed by the connPooler
	closeConn();
}

/**
 * @brief DB::closeConn should be called if you know the db instance is been used in a thread (and ofc will not be used anymore)
 * is not a problem if not done, it will just leave a few warn in the error log likeF
 * [Warning] Aborted connection XXX to db: 'ZYX' user: '123' host: 'something' (Got an error reading communication packets)
 * when the mysql server will forcefully close it
 */
void DB::closeConn() const {
	auto curConn = connPool.get();
	if (curConn) {
		curConn.reset();
	}
}

StMysqlPtr DB::connect() const {
	auto sptr = make_shared<St_mysqlW>();
	// Mysql connection stuff is not thread safe!
	{
		static std::mutex           mutex;
		std::lock_guard<std::mutex> lock(mutex);
		st_mysql*                   conn = mysql_init(nullptr);

		my_bool trueNonSense = 1;
		// looks like is not working very well
		mysql_options(conn, MYSQL_OPT_RECONNECT, &trueNonSense);
		// This will enable non blocking capability
		mysql_options(conn, MYSQL_OPT_NONBLOCK, 0);
		if (conf.compress) {
			// sensibly speed things up if using network, else slow down
			mysql_options(conn, MYSQL_OPT_COMPRESS, &trueNonSense);
		}

		// just spam every where to be sure is used
		mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

		my_bool falseNonSense = 0;

		mysql_options(conn, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &falseNonSense);
		//Needed as of 2022-10 as new mariadb version deprecated older cypher
		mysql_optionsv(conn, MARIADB_OPT_TLS_VERSION, (void*)"TLSv1.2,TLSv1.3");
		//Needed as of 2022-10 as new mariadb requires stronger cypher
		unsigned int cipher_strength = 128;
		mysql_optionsv(conn, MARIADB_OPT_TLS_CIPHER_STRENGTH, (void*)&cipher_strength);

		// Default timeout during connection and operation is Infinite o.O
		// In a real worild if after 5 sec we still have no conn, is clearly an error!
		/*
		uint oldTimeout, readTimeout, writeTimeout;
		mysql_get_option(conn, MYSQL_OPT_CONNECT_TIMEOUT, &oldTimeout);
		mysql_get_option(conn, MYSQL_OPT_READ_TIMEOUT, &readTimeout);
		mysql_get_option(conn, MYSQL_OPT_WRITE_TIMEOUT, &writeTimeout);
		*/

		uint timeout = 10;
		mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
		mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &conf.writeTimeout);
		/**
		 * This is a double edged sword
		 * during long query you will have error 2013
		 * but will avoid to have struct connection too
		 * SADLY is only at session level and can be SET only on connection
		 *
		 * https://stackoverflow.com/questions/34369376/what-is-mysqls-wait-timeout-net-read-timeout-and-net-write-timeout-variable
		 *
		 *Literally is MYSQL_OPT_READ_TIMEOUT: Specifies the timeout in seconds for reading packets from the server.
		 */
		if (conf.readTimeout) {
			mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &conf.readTimeout);
		}

		// just to check we have the conf set
		getConf();

		auto flag = CLIENT_MULTI_STATEMENTS;
		if (conf.ssl) {
			mysql_options(conn, MYSQL_OPT_SSL_ENFORCE, &trueNonSense);
			flag |= CLIENT_SSL;
		}

		// For some reason mysql is now complaining of not having a DB selected... just select one and gg
		auto sock      = conf.sock.has_value() ? conf.sock.value().constData() : nullptr;
		auto connected = mysql_real_connect(conn, conf.host, conf.user.constData(), conf.pass.constData(),
		                                    conf.getDefaultDB(),
		                                    conf.port.value(), sock, flag);
		if (connected == nullptr) {
			auto    errorNo = mysql_errno(conn);
			QString error   = mysql_error(conn);

			if (errorNo == ER_BAD_DB_ERROR) {
				auto& msg = state.get().lastError;
				msg       = F16("Mysql connection error (mysql_init). for {} \n Error {} \n Use a VALID default DB..!", conf.getInfo(), error);

				throw DBException(msg, DBException::Error::InvalidDB);
			}

			// Whoever conceived those api need to search for help -.-
			static const QRegularExpression reg(R"(\((\d*)\))");

			if (auto match = reg.globalMatch(error); match.hasNext()) {
				if (auto v = match.next().captured(1).toInt(); v) {
					error.append(QSL(" / ") + strerror(v));
				}
			}

			auto& msg = state.get().lastError;
			msg       = F16("Mysql connection error (mysql_init). for {} \n Error {} \n Did you forget to enable SSL ?", conf.getInfo(), error);

			mysql_close(conn);
			messanger(msg, conf.connErrorVerbosity);
			throw DBException(msg, DBException::Error::Connection);
		}

		/***/
		sptr->set(conn);
		connPool = sptr;
		/***/
	}

	if (!conf.isMariaDB8.value()) {
		query("SET @@SQL_MODE = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION,ONLY_FULL_GROUP_BY';");
	}

	try {
		query(QBL("SET time_zone='UTC'"));
	} catch (...) {
		qCritical() << R"(
# You are missing the Timezone in the db install the packet with
zypper in mariadb-tools

# be sure the execute the next command as a root user
mariadb_tzinfo_to_sql /usr/share/zoneinfo | mariadb -u root mysql
)";
		throw;
	}

	if (!conf.writeBinlog) {
		query("SET sql_log_bin = 0");
	}

	{
		//this is the normal character, enforced to avoid weird conversion in table
		query("SET collation_server = 'utf8mb4_general_ci';");
	}

	return sptr;
}

bool DB::tryConnect() const {
	try {
		// In try connect. connection error are now very bad...
		cxaLevel = CxaLevel::debug;
		connect();
		return true;
	} catch (const ExceptionV2& e) {
		qDebug() << e.what();
		return false;
	} catch (...) {
		qDebug() << "exception";
		return false;
	}
}

quint64 getId(const sqlResult& res) {
	if (!res.isEmpty()) {
		auto& line = res.at(0);
		auto  iter = line.find(QBL("last_id"));
		if (iter != line.endQt()) {
			bool ok = false;
			auto v2 = iter->toULongLong(&ok);
			if (v2 > 0 && ok) {
				return v2;
			}
		}
	}
	qWarning().noquote() << "error fetching last_id" << QStacker16();
	return 0;
}

QString Q64(const sqlRow& line, const QByteArray& b) {
	return base64this(QV(line, b));
}

QByteArray Q8(const sqlRow& line, const QByteArray& b) {
	return line.value(b);
}

bool DB::isSSL() const {
	auto res = query("SHOW STATUS LIKE 'Ssl_cipher'");
	if (res.isEmpty()) {
		return false;
	} else {
		auto cypher = res[0]["Value"];
		// whatever is set means is ok
		return cypher.length() > 5;
	}
}

void DB::startQuery(const StringAdt& sql) const {
	int  err;
	auto conn  = getConn();
	signalMask = mysql_real_query_start(&err, conn, sql.c_str(), sql.length());
	if (!signalMask) {
		throw QSL("Error executing ASYNC query (start):") + mysql_error(conn);
	}
}

sqlResult DB::getWarning(bool useSuppressionList) const {
	sqlResult ok;
	auto      warnCount = mysql_warning_count(getConn());
	if (!warnCount) {
		return ok;
	}
	auto res = query(QBL("SHOW WARNINGS"));
	if (!useSuppressionList || conf.warningSuppression.empty()) {
		return res;
	}
	for (const auto& row : res) {
		auto msg = row.value(QBL("Message"), BSQL_NULL);
		for (auto& rx : conf.warningSuppression) {
			//auto p = rx->pattern();
			if (rx->match(msg).hasMatch()) {
				break;
			} else {
				ok.append(row);
			}
		}
	}
	return ok;
}

sqlResult DB::fetchResult(SQLLogger* sqlLogger) const {
	QElapsedTimer timer;
	timer.start(); // this will be stopped in the destructor of sql logger
	// most inefficent way, but most easy to use!
	sqlResult res;
	res.reserve(512);

	auto conn = getConn();
	// If you batch more than two select, you are crazy, it is POSSIBLE, but there are SCARCE real use case for it,
	// and will just break this library logic / will require much more metadata and a custom fetchResult
	// just the first one will be returned and you will be in bad situation later
	// this iteration is just if you batch mulitple update, result is NULL, but mysql insist that you fetch them...
	bool         first      = true;
	unsigned int num_fields = 0;
	MYSQL_FIELD* fields     = nullptr;
	do {
		// swap the whole result set we do not expect 1Gb+ result set here
		MYSQL_RES* result = mysql_store_result(conn);

		if (result) {
			if (first) {
				first      = false;
				num_fields = mysql_num_fields(result);
				fields     = mysql_fetch_fields(result);
				if (state->swapType) {
					for (uint16_t i = 0; i < num_fields; i++) {
						auto& field = fields[i];
						res.types.insert({field.name, field.type});
					}
					state->swapType = false;
				}
			}

			my_ulonglong row_count = mysql_num_rows(result);
			for (uint j = 0; j < row_count; j++) {
				MYSQL_ROW row = mysql_fetch_row(result);

				sqlRow thisItem;
				auto   lengths = mysql_fetch_lengths(result);
				for (uint16_t i = 0; i < num_fields; i++) {
					auto& field = fields[i];
					// this is how sql NULL is signaled, instead of having a wrapper and check ALWAYS before access, we normally just ceck on result swap if a NULL has any sense here or not.
					// Plus if you have the string NULL in a DB you are really looking for trouble
					if (row[i] == nullptr && lengths[i] == 0) {
						auto& v = state.get();
						if (v.NULL_as_EMPTY) {
							thisItem.insert(field.name, QByteArray());
						} else {
							thisItem.insert(field.name, BSQL_NULL);
						}
					} else {
						thisItem.insert(field.name, QByteArray(row[i], static_cast<int>(lengths[i])));
					}
				}
				res.push_back(thisItem);
			}
			mysql_free_result(result);
		}
	} while (mysql_next_result(conn) == 0);
	sqlLogger->fetchTime = timer.nsecsElapsed();
	auto& st             = state.get();
	st.fetchTime         = sqlLogger->fetchTime;
	st.totFetchTime += sqlLogger->fetchTime;

	localThreadStatus->time.addSqlTime(sqlLogger->fetchTime + sqlLogger->serverTime);

	affectedRows = mysql_affected_rows(conn);

	// auto affected  = mysql_affected_rows(conn);
	if (skipWarning) {
		// reset
		skipWarning = false;
	} else {
		auto warn = this->getWarning(true);
		if (!warn.isEmpty()) {
			qDebug().noquote() << F16("warning for {} \n{}\n{}", state->lastSQL, warn, stacker());
		}
	}

	unsigned int error = mysql_errno(conn);
	if (error && sqlLogger) {
		sqlLogger->error = mysql_error(conn);
		//There should be a better way...
		sqlLogger->res = res;
	}

	if (error) {
		auto msg = fmt::format(R"(Mysql error for:
{}
----------
Error was:
{}
Code:{}
Query: {:.3f}	Fetch: {:.3f} )",
		                       state->lastSQL, mysql_error(conn), error, (double)sqlLogger->serverTime / 1E9, (double)sqlLogger->fetchTime / 1E9);
		throw ExceptionV2(msg);
	}

	return res;
}

SqlResultV2 DB::fetchResultV2(SQLLogger* sqlLogger) const {
	QElapsedTimer timer;
	timer.start(); // this will be stopped in the destructor of sql logger
	SqlResultV2 res;
	res.reserve(512);

	auto conn = getConn();
	// If you batch more than two select, you are crazy, it is POSSIBLE, but there are SCARCE real use case for it,
	// and will just break this library logic / will require much more metadata and a custom fetchResult
	// just the first one will be returned and you will be in bad situation later
	// this iteration is just if you batch mulitple update, result is NULL, but mysql insist that you fetch them...
	bool         first      = true;
	unsigned int num_fields = 0;
	MYSQL_FIELD* fields     = nullptr;
	do {
		// swap the whole result set we do not expect 1Gb+ result set if we use this function
		MYSQL_RES* result = mysql_store_result(conn);

		if (result) {
			if (first) {
				first      = false;
				num_fields = mysql_num_fields(result);
				fields     = mysql_fetch_fields(result);
				for (uint16_t i = 0; i < num_fields; i++) {
					auto& field = fields[i];
					res.columns->insert({field.name, {field.type, i}});
				}
			}

			my_ulonglong row_count = mysql_num_rows(result);
			for (uint j = 0; j < row_count; j++) {
				MYSQL_ROW row = mysql_fetch_row(result);

				SqlRowV2 thisItem;
				thisItem.columns = res.columns;
				auto lengths     = mysql_fetch_lengths(result);
				for (uint16_t i = 0; i < num_fields; i++) {
					//auto& field = fields[i];
					// this is how sql NULL is signaled, instead of having a wrapper and check ALWAYS before access, we normally just ceck on result swap if a NULL has any sense here or not.
					// Plus if you have the string NULL in a DB you are really looking for trouble
					if (row[i] == nullptr && lengths[i] == 0) {
						if (state.get().NULL_as_EMPTY) {
							thisItem.data.push_back({});
						} else {
							thisItem.data.push_back(S_SQL_NULL);
						}
					} else {
						thisItem.data.push_back(std::string(row[i], static_cast<int>(lengths[i])));
					}
				}
				res.push_back(thisItem);
			}
			mysql_free_result(result);
		}
	} while (mysql_next_result(conn) == 0);
	sqlLogger->fetchTime = timer.nsecsElapsed();
	auto& st             = state.get();
	st.fetchTime         = sqlLogger->fetchTime;
	st.totFetchTime += sqlLogger->fetchTime;

	localThreadStatus->time.addSqlTime(sqlLogger->fetchTime + sqlLogger->serverTime);

	affectedRows = mysql_affected_rows(conn);

	// auto affected  = mysql_affected_rows(conn);
	if (skipWarning) {
		// reset
		skipWarning = false;
	} else {
		auto warn = this->getWarning(true);
		if (!warn.isEmpty()) {
			qDebug().noquote() << F16("warning for {} \n{}\n{}", state->lastSQL, warn, stacker());
		}
	}

	unsigned int error = mysql_errno(conn);
	if (error && sqlLogger) {
		sqlLogger->error = mysql_error(conn);
		//There should be a better way...
		//sqlLogger->res = res;
	}

	if (error) {
		auto msg = fmt::format(R"(Mysql error for:
{}
----------
Error was:
{}
Code:{}
Query: {:.3f}	Fetch: {:.3f} )",
		                       state->lastSQL, mysql_error(conn), error, (double)sqlLogger->serverTime / 1E9, (double)sqlLogger->fetchTime / 1E9);
		throw ExceptionV2(msg);
	}

	return res;
}

//Linux only ATM
#if __has_include(<poll.h>)
#include <poll.h>

bool DB::completedQuery() const {
	auto conn = getConn();

	auto error = mysql_errno(conn);
	if (error != 0) {
		qWarning().noquote() << F16("Mysql error for {} error was {} code: {}\n{}", state->lastSQL, mysql_error(conn), error, stacker(3));
		throw 1025;
	}
	int err;

	auto event = somethingHappened(conn, signalMask);
	if (event) {
		event = mysql_real_query_cont(&err, conn, event);
		if (err) {
			throw QSL("Error executing ASYNC query (cont):") + mysql_error(conn);
		}
		// if we are still listening to an event, return false
		// else if we have no more event to wait return true
		return !event;
	} else {
		return false;
	}
}

u64 DB::fetchAdvanced(FetchVisitor* visitor) const {
	auto conn = getConn();

	MYSQL_RES* result = mysql_use_result(conn);
	if (!result) {
		auto error = mysql_errno(conn);
		if (error != 0) {
			qWarning().noquote() << F16("Mysql error for {} error was {} code: {}\n{}", state->lastSQL, mysql_error(conn), error, stacker(3));
			cxaNoStack = true;
			throw 1025;
		}
	}

	if (!visitor->preCheck(result)) {
		//???
		throw QSL("no idea what to do whit this result set!");
	}
	while (auto row = mysql_fetch_row(result)) {
		visitor->processLine(row);
	}
	auto rowCount = mysql_num_rows(result);
	mysql_free_result(result);
	return rowCount;
}

/**
  why static ? -> https://stackoverflow.com/a/15235626/1040618
  in short is not exported
 * @brief wait_for_mysql
 * @param mysql
 * @param status
 * @return
 */
static int somethingHappened(MYSQL* mysql, int status) {
	struct pollfd pfd;
	int           res;

	pfd.fd = mysql_get_socket(mysql);
	pfd.events =
	    (status & MYSQL_WAIT_READ ? POLLIN : 0) |
	    (status & MYSQL_WAIT_WRITE ? POLLOUT : 0) |
	    (status & MYSQL_WAIT_EXCEPT ? POLLPRI : 0);

	// We have no reason to wait, either is ready or not
	res = poll(&pfd, 1, 0);
	if (res == 0)
		return 0;
	else if (res < 0) {
		return 0;
	} else {
		int _status = 0;
		if (pfd.revents & POLLIN)
			_status |= MYSQL_WAIT_READ;
		if (pfd.revents & POLLOUT)
			_status |= MYSQL_WAIT_WRITE;
		if (pfd.revents & POLLPRI)
			_status |= MYSQL_WAIT_EXCEPT;
		return _status;
	}
}
#endif

SQLLogger::SQLLogger(const string& _sql, bool _enabled, const DB* _db)
    : sql(_sql),
      logError(_enabled),
      db(_db) {
}

void SQLLogger::flush() {
	if (flushed) {
		return;
	}
	//If no error, and no logSql
	if (error.empty() && !logSql) {
		return;
	}
	//If there is an error but logError is off
	if (!error.empty() && !logError) {
		return;
	}
	flushed = true;
	static std::mutex            lock;
	std::scoped_lock<std::mutex> scoped(lock);

	// we keep open a file and just append from now on...
	// for the moment is just a single file later... who knows
	static QFile file;
	if (!file.isOpen()) {
		file.setFileName(QSL("sql_%1.log").arg(db->getConf().cacheId));
		if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
			qWarning().noquote() << "impossible to open sql.log";
			return;
		}
	}

	QDateTime myDateTime = QDateTime::currentDateTime();
	QString   time       = myDateTime.toString(Qt::ISODateWithMs);
	file.write(time.toUtf8() + "\n");

	auto pid = getpid();

	unsigned long mysqlThreadId = 0;
	//if it happen that the connection goes down, than we have a deadlock IF we try to reconnect, so skip reconnection
	if (auto dbConn = db->getConn(true); dbConn) {
		mysqlThreadId = mysql_thread_id(dbConn);
	}

	auto info = F("PID: {}, MySQL Thread: {} \n", pid, mysqlThreadId);
	{
		double query = static_cast<double>(serverTime) / 1E9;
		double fetch = static_cast<double>(fetchTime) / 1E9;
		info += F("Query: {:.3e} Fetch: {:.3e}\n{}", query, fetch, sql);
	}

	if (!error.empty()) {
		info += "\nError: " + error;

		if (!res.isEmpty()) {
			file.write("\n");
			// nice trick to use qDebug operator << on a custom stream!
			QDebug dbg(&file);
			dbg << (res);
		}
	}

	file.write(info.c_str(), info.size());
	file.write("\n-------------\n");
	file.flush();
}

SQLLogger::~SQLLogger() {
	flush();
}

QString nullOnZero(uint v) {
	if (v) {
		return QString::number(v);
	} else {
		return SQL_NULL;
	}
}

/**
 * to check reconnection
 * 	while (true) {
                try {
                        qDebug() << s7Db.queryLine("SELECT NOW(6)");
                } catch (...) {
                        qDebug() << "Query error";
                }

                usleep(1E5);
        }

        exit(1);
        */

QDateTime sqlRow::asDateTime(const QByteArray& key) const {
	auto time = get2<QString>(key);
	return QDateTime::fromString(time, mysqlDateTimeFormat);
}

QString sqlRow::serialize() const {
	// Almost free operator <<
	QString out;
	QDebug  dbg(&out);
	dbg << (*this);
	return out;
}

DBException::DBException(const StringAdt& _msg, Error error)
    : ExceptionV2(_msg) {
	errorType = error;
}

QString base64Nullable(const QString* param, bool emptyAsNull) {
	if (param) {
		return base64Nullable(*param, emptyAsNull);
	} else if (emptyAsNull) {
		return SQL_NULL;
	} else {
		return "";
	}
}

QString asString(const sqlRow& row, bool noQuote) {
	QString s;
	if (noQuote) {
		QDebug(&s).noquote() << row;
	} else {
		QDebug(&s) << row;
	}

	return s;
}

QStringList getIdList(const sqlResult& sqlRes, const QString& idName) {
	QStringList rangeIdList;
	for (auto& row : sqlRes) {
		auto id = row.get2<QString>(idName.toUtf8());
		rangeIdList.push_back(id);
	}
	return rangeIdList;
}

MyType::MyType(const enum_field_types& t) {
	type = t;
}

bool MyType::isNumeric() const {
	return IS_NUM(type);
}

bool MyType::isFloat() const {
	switch (type) {
	case enum_field_types::MYSQL_TYPE_DECIMAL:
	case enum_field_types::MYSQL_TYPE_DOUBLE:
	case enum_field_types::MYSQL_TYPE_FLOAT:
		return true;
	default:
		return false;
	}
}

St_mysqlW::operator st_mysql*() {
	return conn;
}

void St_mysqlW::set(st_mysql* c) {
	if (conn) {
		throw ExceptionV2("NO! you can not reuse this class!");
	}
	++connCounter;
	conn = c;
}

St_mysqlW::~St_mysqlW() {
	if (conn) {
		--connCounter;
		mysql_close(conn);
		conn = nullptr;
	}
}

SqlResultV2 DB::queryV2(const StringAdt& sql) {
	if (sql.empty()) {
		return {};
	}

	auto logger = queryInner(sql);

	if (noFetch) {
		return {};
	}

	return fetchResultV2(&logger);
}
