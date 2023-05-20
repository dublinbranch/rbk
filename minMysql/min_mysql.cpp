#include "min_mysql.h"
#include "mysql/mysql.h"
#include "mysql/mysqld_error.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII//resetAfterUse.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"
#include "rbk/fmtExtra/customformatter.h"
#include "rbk/hash/sha.h"
#include "rbk/misc/b64.h"
#include "rbk/serialization/serialize.h"
#include "rbk/thread/threadstatush.h"
#include "utilityfunctions.h"
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QScopeGuard>
#include <memory>
#include <mutex>
#include <poll.h>
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
// I (Roy) really do not like reading warning, so we will now properly close all opened connection!
class ConnPooler {
      public:
	void                        addConnPool(st_mysql* conn);
	void                        removeConn(st_mysql* conn);
	void                        closeAll();
	const map<st_mysql*, bool>& getPool() const;
	~ConnPooler();

	ulong active = 0xBADF00DBADC0FFEE;

      private:
	map<st_mysql*, bool> allConn;
	mutex                allConnMutex;
};

static ConnPooler connPooler;
static int        somethingHappened(MYSQL* mysql, int status);

sqlRow DB::queryLine(const char* sql) const {
	return queryLine(QByteArray(sql));
}

sqlRow DB::queryLine(const QString& sql) const {
	return queryLine(sql.toUtf8());
}

sqlRow DB::queryLine(const std::string& sql) const {
	QByteArray temp;
	temp.setRawData(sql.c_str(), sql.size());
	return queryLine(temp);
}

sqlRow DB::queryLine(const QByteArray& sql) const {
	auto res = query(sql);
	if (res.empty()) {
		return sqlRow();
	}
	return res[0];
}

void DB::setMaxQueryTime(uint time) const {
	query(QSL("SET @@max_statement_time  = %1").arg(time));
}

sqlResult DB::query(const QString& sql) const {
	//I have no idea but libasan reported the error if is inlined o.O ?
	auto copy = sql.toUtf8();
	return query(copy);
}

sqlResult DB::query(const std::string& sql) const {
	return query(QByteArray::fromStdString(sql));
}

sqlResult DB::query(const QByteArray& sql, int simulateErr) const {
	ResetAfterUse reset1(localThreadStatus->state, ThreadState::MyQuery);
	localThreadStatus->sql = sql;

	if (sql.isEmpty()) {
		return {};
	}
	auto conn = getConn();
	if (conn == nullptr) {
		throw ExceptionV2("This mysql instance is not connected!");
	}

	SQLLogger sqlLogger(sql, conf.logError, this);
	if (sql != "SHOW WARNINGS") {
		lastSQL          = sql;
		sqlLogger.logSql = conf.logSql;
	} else {
		sqlLogger.logSql = false;
	}

	pingCheck(conn);

	{
		QElapsedTimer timer;
		timer.start();

		sharedState.busyConnection++;
		mysql_query(conn, sql.constData());
		sharedState.busyConnection--;
		sqlLogger.serverTime = timer.nsecsElapsed();
		auto& st             = state.get();
		st.queryExecuted++;
		st.serverTime = sqlLogger.serverTime;
		st.totServerTime += sqlLogger.serverTime;
	}

	auto error = mysql_errno(conn);
	if (simulateErr) {
		error = simulateErr;
	}

	if (error) {
		switch (error) {
		case 1062: { //unique index violation
			cxaNoStack     = true;
			cxaLevel       = CxaLevel::none;
			auto exception = DBException(mysql_error(conn), DBException::Error(error));
			throw exception;
		}
		case 1065:
			// well an empty query is bad, but not too much!
			qWarning().noquote() << "empty query (or equivalent for) " << sql << "in" << QStacker16();
			return sqlResult();

		// Lock wait timeout exceeded; try restarting transaction
		case 1205:
		// Deadlock found when trying to get lock; try restarting transaction
		case 1213:
		// Lost connection to MySQL server during query
		case 2013: {
			QString err;
			if (state.get().skipNextDisconnect) {
				state.get().skipNextDisconnect = false;
			} else {
				// This is sometimes happening, and I really have no idea how to fix, there is already the ping at the beginning, but looks like is not working...
				// so we try to get some info

				QString errSkel = R"(
	Mysql error for %1
	error was %2 code: %3, connInfo: %4
	thread: %5, queryDone: %6, reconnection: %7, busyConn: %8, totConn: %9, queryTime: %10 nanoseconds (%11 nanoseconds)
	%12
	)";
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

				err = errSkel
				          .arg(QString(sql))
				          .arg(mysql_error(conn))
				          .arg(error)
				          .arg(conf.getInfo())
				          .arg(mysql_thread_id(conn))
				          .arg(state.get().queryExecuted)
				          .arg(state.get().reconnection)
				          .arg(sharedState.busyConnection)
				          .arg(connPooler.getPool().size())
				          .arg((double)sqlLogger.serverTime, 0, 'G', 3)
				          .arg(sqlLogger.serverTime)
				          .arg(runningSqls);
				sqlLogger.error = err;

				qWarning().noquote() << err << QStacker16();
			}

			closeConn();

			cxaNoStack     = true;
			cxaLevel       = CxaLevel::none;
			auto exception = DBException(err, DBException::Error(error));
			throw exception;
		} break;
		default:
			auto err = QSL("Mysql error for %1 \nerror was %2 code: %3\n%4")
			               .arg(QString(sql))
			               .arg(mysql_error(conn))
			               .arg(error)
			               .arg(getConf().getInfo());
			sqlLogger.error = err;
			// this line is needed for proper email error reporting
			qWarning().noquote() << err << QStacker16();
			cxaNoStack     = true;
			auto exception = DBException(err, DBException::Error::NA);
			throw exception;
		}
	}

	if (noFetch) {
		return sqlResult();
	}
	return fetchResult(&sqlLogger);
}

sqlResult DB::queryCache(const QString& sql, bool on, QString name, uint ttl) {
	(void)on;
	(void)name;
	return queryCache2(sql, ttl);
}

sqlRow DB::queryCacheLine(const QString& sql, bool on, QString name, uint ttl, bool required) {
	(void)on;
	(void)name;
	return queryCacheLine2(sql, ttl, required);
}

sqlRow DB::queryCacheLine2(const QString& sql, uint ttl, bool required) {
	//TODO forward the check into query cache line ?
	auto res = queryCache2(sql, ttl);
	if (auto r = res.size(); r > 1) {
		auto   msg = QSL("invalid number of row for: %1\nExpected 1, got %2 \n").arg(sql).arg(r);
		QDebug dbg(&msg);
		for (int i = 0; i < 3; i++) {
			if (!res.empty()) {
				dbg.noquote() << res.takeFirst() << "\n--------------------\n";
			}
		}

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

sqlRow DB::queryCacheLine2(const std::string& sql, uint ttl, bool required) {
	return queryCacheLine2(QString::fromStdString(sql), ttl, required);
}

sqlResult DB::queryCache2(const std::string& sql, const Opt& opt) {
	state.get().noCacheOnEmpty = opt.noCacheOnEmpty;
	return queryCache2(QString::fromStdString(sql), opt.ttl, opt.required);
}

sqlResult DB::queryCache2(const std::string& sql, uint ttl, bool required) {
	return queryCache2(QString::fromStdString(sql), ttl, required);
}

sqlRow DB::queryCacheLine(const QString& sql, uint ttl, bool required) {
	return queryCacheLine2(sql, ttl, required);
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

sqlResult DB::queryCache2(const QString& sql, uint ttl, bool required) {
	ResetAfterUse<typeof localThreadStatus->state> reset1;
	if (localThreadStatus) {
		reset1.set(localThreadStatus->state, ThreadState::MyCache);
	}

	SetOnExit noCache(state.get().noCacheOnEmpty, false);
	if (ttl) {
		// small trick to avoid calling over and over the function
		static bool fraud = mkdir(QSL("cachedSQL_%1/").arg(conf.cacheId));
		(void)fraud;

		//We have a lock to prevent concurrent write in this process, but nothing to protect again other, just use another folder
		QString                      name = QSL("cachedSQL_%1/").arg(conf.cacheId) + sha1(sql);
		static std::mutex            lock;
		std::scoped_lock<std::mutex> scoped(lock);

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

		lock.unlock();
		res = query(sql);
		lock.lock();
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

sqlResult DB::queryORcache(const QString& sql, uint ttl, bool required) {
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
	if (!conf.pingBeforeQuery) {
		return;
	}
	SQLLogger sqlLogger("PING", conf.logError, this);
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
		auto err   = QSL("Mysql error for %1 \nerror was %2 code: %3, connRetry for %4, connectionId: %5, conf: ")
		               .arg(QString(sqlLogger.sql))
		               .arg(mysql_error(conn))
		               .arg(error)
		               .arg(connRetry)
		               .arg(mysql_error(conn)) +
		           conf.getInfo() +
		           QStacker16();
		sqlLogger.error = err;
		// this line is needed for proper email error reporting
		qWarning().noquote() << err;
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
	char* tStr = new char[plain.size() * 2 + 1];
	mysql_real_escape_string(getConn(), tStr, plain.constData(), plain.size());
	auto escaped = QString::fromUtf8(tStr);
	delete[] tStr;
	return escaped;
}

string DB::escape(const std::string& plain) const {
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
	st_mysql* curConn = connPool;
	if (curConn == nullptr) {
		if (doNotConnect) {
			return nullptr;
		}
		// loading in connPool is inside
		curConn = connect();
	}
	return curConn;
}

ulong DB::lastId() const {
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
		auto msg = QSL("default DB is sadly required to avoid mysql complain on certain operation!") + QStacker16Light();
		qWarning().noquote() << msg;
		cxaNoStack = true;
		throw msg;
	}
	return defaultDB;
}

void DBConf::setDefaultDB(const QByteArray& value) {
	defaultDB = value;
}

QString DBConf::getInfo(bool passwd) const {
	auto msg = QSL(" %1:%2  user: %3")
	               .arg(QString(host))
	               .arg(port)
	               .arg(user.data());
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
 */
void DB::closeConn() const {
	st_mysql* curConn = connPool;
	if (curConn) {
		// this whole conn pool architecture is wrong, ditch it and recreate something and do not realy on magic constant
		// to detect if something is freed or not
		if (connPooler.active == 0xBADF00DBADC0FFEE) {
			connPooler.removeConn(curConn);
			mysql_close(curConn);
		}
		connPool = nullptr;
	}
}

st_mysql* DB::connect() const {
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
		// sensibly speed things up
		mysql_options(conn, MYSQL_OPT_COMPRESS, &trueNonSense);
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
		mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
		/**
		 * This is a double edged sword
		 * during long query you will have error 2013
		 * but will avoid to have struct connection too
		 * SADLY is only at session level and can be SET only on connection
		 *
		 * https://stackoverflow.com/questions/34369376/what-is-mysqls-wait-timeout-net-read-timeout-and-net-write-timeout-variable
		 *
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
		auto connected = mysql_real_connect(conn, conf.host, conf.user.constData(), conf.pass.constData(),
		                                    conf.getDefaultDB(),
		                                    conf.port, conf.sock.constData(), flag);
		if (connected == nullptr) {
			auto    errorNo = mysql_errno(conn);
			QString error   = mysql_error(conn);

			if (errorNo == ER_BAD_DB_ERROR) {
				auto& msg = state.get().lastError;
				msg       = QSL("Mysql connection error (mysql_init). for %1 \n Error %2 \n Use a VALID default DB..!")
				          .arg(conf.getInfo())
				          .arg(error);

				throw DBException(msg, DBException::Error::InvalidDB);
			}

			// Whoever conceived those api need to search for help -.-
			static const QRegularExpression reg(R"(\((\d*)\))");

			if (auto match = reg.globalMatch(error); match.hasNext()) {
				if (auto v = match.next().captured(1).toUInt(); v) {
					error.append(QSL(" / ") + strerror(v));
				}
			}

			auto& msg = state.get().lastError;
			msg       = QSL("Mysql connection error (mysql_init). for %1 \n Error %2 \n Did you forget to enable SSL ?")
			          .arg(conf.getInfo())
			          .arg(error);

			mysql_close(conn);
			messanger(msg, conf.connErrorVerbosity);
			throw DBException(msg, DBException::Error::Connection);
		}

		/***/
		connPool = conn;
		connPooler.addConnPool(conn);
		/***/
	}

	if (!conf.noSqlMode) {
		query(QBL("SET @@SQL_MODE = 'STRICT_TRANS_TABLES,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,ONLY_FULL_GROUP_BY';"));
	}

	query(QBL("SET time_zone='UTC'"));
	if (!conf.writeBinlog) {
		query(QBL("SET sql_log_bin = 0"));
	}

	//this function is normally called when a new instance is created in a MT program, so we have to set again the local state
	state.get().NULL_as_EMPTY = conf.NULL_as_EMPTY;
	return connPool;
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

SQLBuffering::SQLBuffering(DB* _conn, uint _bufferSize, bool _useTRX) {
	this->conn       = _conn;
	this->bufferSize = _bufferSize;
	this->useTRX     = _useTRX;
}

SQLBuffering::~SQLBuffering() {
	try {
		flush();
	} catch (std::exception& e) {
		qCritical() << e.what();
	} catch (...) {
		qCritical() << "unknow exception in " << QStacker16();
	}
}

void SQLBuffering::append(const std::string& sql) {
	append(QString::fromStdString(sql));
}

void SQLBuffering::append(const QString& sql) {
	if (sql.isEmpty()) {
		return;
	}

	// 0 disable flushing, 1 disable buffering
	if (bufferSize && (uint)buffer.size() >= bufferSize) {
		flush();
	}
	buffer.append(sql);
}

void SQLBuffering::append(const QStringList& sqlList) {
	for (auto& row : sqlList) {
		append(row);
	}
}

void SQLBuffering::flush() {
	if (buffer.isEmpty()) {
		return;
	}
	if (conn == nullptr) {
		throw QSL("you forget to set a usable DB Conn!") + QStacker16();
	}
	/**
	 * To avoid having a very big packet we split
	 * usually max_allowed_packet is https://mariadb.com/kb/en/server-system-variables/#max_allowed_packet
	   16777216 (16M) >= MariaDB 10.2.4
	        4194304 (4M) >= MariaDB 10.1.7
	        1048576 (1MB) <= MariaDB 10.1.6
	 1073741824 (1GB) (client-side)
	 It should be nice to read the value from the conn ... ?
	 TODO add
	 show variables like "max_allowed_packet"
	 */

	// This MUST be out of the buffered block!
	if (useTRX) {
		conn->query(QBL("START TRANSACTION;"));
	}

	QString query;
	// TODO just compose the query in utf8, and append in utf8
	for (auto&& line : buffer) {
		query.append(line);
		query.append(QSL("\n"));
		// this is UTF16, but MySQL run in UTF8, so can be lower or bigger (rare vey rare but possible)
		// small safety margin + increase size for UTF16 -> UTF8 conversion
		if ((query.size() * 1.3) > maxPacket * 0.75) {
			conn->queryDeadlockRepeater(query.toUtf8());
			query.clear();
		}
	}
	buffer.clear();
	if (!query.isEmpty()) {
		conn->queryDeadlockRepeater(query.toUtf8());
	}
	// This MUST be out of the buffered block!
	if (useTRX) {
		conn->query(QBL("COMMIT;"));
	}
}

void SQLBuffering::setUseTRX(bool _useTRX) {
	this->useTRX = _useTRX;
}

void SQLBuffering::clear() {
	buffer.clear();
}

QString Q64(const sqlRow& line, const QByteArray& b) {
	return base64this(QV(line, b));
}

QByteArray Q8(const sqlRow& line, const QByteArray& b) {
	return line.value(b);
}

sqlResult DB::query(const char* sql) const {
	return query(QByteArray(sql));
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

void DB::startQuery(const QByteArray& sql) const {
	int  err;
	auto conn  = getConn();
	signalMask = mysql_real_query_start(&err, conn, sql.constData(), sql.length());
	if (!signalMask) {
		throw QSL("Error executing ASYNC query (start):") + mysql_error(conn);
	}
}

void DB::startQuery(const QString& sql) const {
	return startQuery(sql.toUtf8());
}

void DB::startQuery(const char* sql) const {
	return startQuery(QByteArray(sql));
}

bool DB::completedQuery() const {
	auto conn = getConn();

	auto error = mysql_errno(conn);
	if (error != 0) {
		qWarning().noquote() << "Mysql error for " << lastSQL << "error was " << mysql_error(conn) << " code: " << error << QStacker(3);
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
				for (uint16_t i = 0; i < num_fields; i++) {
					auto& field = fields[i];
					res.types.insert({field.name, field.type});
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
						if (state.get().NULL_as_EMPTY) {
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
			qDebug().noquote() << "warning for " << lastSQL << warn << "\n"
			                   << QStacker16Light();
		}
	}

	unsigned int error = mysql_errno(conn);
	if (error && sqlLogger) {
		sqlLogger->error = mysql_error(conn);
		//There should be a better way...
		sqlLogger->res = res;
	}

	if (error) {
		auto msg = fmt::format("Mysql error for:\n{} \n----------\nError was:\n{}\nCode:{}", lastSQL.get(), mysql_error(conn), error);
		throw ExceptionV2(msg);
	}

	return res;
}

int DB::fetchAdvanced(FetchVisitor* visitor) const {
	auto conn = getConn();

	// swap the whole result set we do not expect 1Gb+ result set here
	MYSQL_RES* result = mysql_use_result(conn);
	if (!result) {
		auto error = mysql_errno(conn);
		if (error != 0) {
			qWarning().noquote() << "Mysql error for " << lastSQL << "error was " << mysql_error(conn) << " code: " << error << QStacker(3);
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
	mysql_free_result(result);
	// no idea what to return
	return 1;
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

SQLLogger::SQLLogger(const QByteArray& _sql, bool _enabled, const DB* _db)
    : sql(_sql),
      logError(_enabled),
      db(_db) {
}

void SQLLogger::flush() {
	if (flushed) {
		return;
	}
	if (!(logError || logSql)) {
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

	QString info = QSL("PID: %1, MySQL Thread: %2 \n").arg(pid).arg(mysqlThreadId);

	file.write(info.toUtf8());

	double     query = serverTime / 1E9;
	double     fetch = fetchTime / 1E9;
	QByteArray buff  = "Query: " + QByteArray::number(query, 'E', 3);
	file.write(buff.leftJustified(20, ' ').append("Fetch: " + QByteArray::number(fetch, 'E', 3)) + "\n" + sql);
	if (!error.isEmpty()) {
		file.write(QBL("\nError: ") + error.toUtf8());
		if (!res.isEmpty()) {
			file.write("\n");
			// nice trick to use qDebug operator << on a custom stream!
			QDebug dbg(&file);
			dbg << (res);
		}
	}

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

void ConnPooler::addConnPool(st_mysql* conn) {
	lock_guard<mutex> guard(allConnMutex);
	allConn.insert({conn, true});
}

void ConnPooler::removeConn(st_mysql* conn) {
	lock_guard<mutex> guard(allConnMutex);
	if (auto iter = allConn.find(conn); iter != allConn.end()) {
		allConn.erase(iter);
	}
}

void ConnPooler::closeAll() {
	lock_guard<mutex> guard(allConnMutex);
	for (auto& [conn, dummy] : allConn) {
		mysql_close(conn);
	}
	allConn.clear();
}

const map<st_mysql*, bool>& ConnPooler::getPool() const {
	return allConn;
}

ConnPooler::~ConnPooler() {
	active = 0;
	closeAll();
}

DBException::DBException(const QString& _msg, Error error)
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

QString asString(const sqlRow& row) {
	QString s;
	QDebug(&s) << row;
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

MyType::MyType(enum_field_types& t) {
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
