#pragma once

#include "DBConf.h"
#include "MITLS.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/mapExtensor/mapV2.h"
#include "sqlRow.h"
#include "sqlresult.h"
#include <QDateTime>
#include <QDebug>
#include <QStringList>
#include <mysql/mysql.h>

class DBException : public ExceptionV2 {
      public:
	enum Error : int {
		NA = 0,
		Warning,
		SchemaError,
		NoResult,
		InvalidDB  = 1049,
		DeadLock   = 1213,
		Duplicate  = 1062,
		Connection = 2013

	} errorType = Error::NA;
	DBException(const QString& _msg, Error error);
};

QString nullOnZero(uint v);

struct st_mysql;
struct st_mysql_res;

QString asString(const sqlRow& row);

QStringList getIdList(const sqlResult& sqlRes, const QString& idName);

class DB;
struct DBConf;

struct SQLLogger {
	SQLLogger(const QByteArray& _sql, bool _enabled, const DB* _db);
	void flush();
	~SQLLogger();

	qint64           serverTime;
	qint64           fetchTime;
	const QByteArray sql;
	sqlResult        res;
	QString          error;
	// TODO questi due leggili dal conf dell db*
	bool logSql   = false;
	bool logError = false;
	bool flushed  = false;
	// the invoking class
	const DB* db = nullptr;
};

/**
 * @brief The DB struct
 */
class FetchVisitor;

class DB {
      public:
	struct Opt {
		uint ttl            = 60;
		bool noCacheOnEmpty = false;
		bool required       = false;
	};

	DB() = default;
	DB(const DBConf& _conf);
	~DB();
	void      closeConn() const;
	st_mysql* connect() const;
	bool      tryConnect() const;

	sqlRow queryLine(const std::string& sql) const;
	sqlRow queryLine(const char* sql) const;
	sqlRow queryLine(const QString& sql) const;
	sqlRow queryLine(const QByteArray& sql) const;

	void      setMaxQueryTime(uint time) const;
	sqlResult query(const char* sql) const;
	sqlResult query(const QString& sql) const;
	sqlResult query(const std::string& sql) const;
	// simulateErr is just for testing
	sqlResult query(const QByteArray& sql, int simulateErr = 0) const;

	[[deprecated("use queryCache2 - this one is problematic to use, and with redundant and never used param")]] sqlResult  queryCache(const QString& sql, bool on = false, QString name = QString(), uint ttl = 3600);
	[[deprecated("use queryCacheLine2 - this one is problematic to use, and with redundant and never used param")]] sqlRow queryCacheLine(const QString& sql, bool on = false, QString name = QString(), uint ttl = 3600, bool required = false);

	sqlRow queryCacheLine(const QString& sql, uint ttl = 3600, bool required = false);
	sqlRow queryCacheLine2(const QString& sql, uint ttl = 3600, bool required = false);
	sqlRow queryCacheLine2(const std::string& sql, uint ttl = 3600, bool required = false);

	sqlResult queryCache2(const std::string& sql, const Opt& opt);

	sqlResult queryCache2(const std::string& sql, uint ttl, bool required = false);
	sqlResult queryCache2(const QString& sql, uint ttl, bool required = false);
	//Try to read data from cache, if expired read from DB, if db unavailable use the cache, if all fail throw error
	sqlResult queryORcache(const QString& sql, uint ttl, bool required = false);

	// This is to be used ONLY in case the query can have deadlock, and internally tries multiple times to insert data
	sqlResult queryDeadlockRepeater(const QByteArray& sql, uint maxTry = 5) const;

	void        pingCheck(st_mysql*& conn) const;
	QString     escape(const QString& what) const;
	std::string escape(const std::string& what) const;
	bool        isSSL() const;
	/**
	  Those 2 are used toghether for the ASYNC mode
	 * @brief startQuery
	 * @param sql
	 *
	 */
	void startQuery(const QByteArray& sql) const;
	void startQuery(const QString& sql) const;
	void startQuery(const char* sql) const;
	/** use something like
	        while (!db.completedQuery()) {
	                usleep(100);
	        }
	        fetch
	 * @brief completedQuery
	 * @return
	 */
	bool completedQuery() const;

	// Shared by both async and not
	sqlResult getWarning(bool useSuppressionList = true) const;
	sqlResult fetchResult(SQLLogger* sqlLogger = nullptr) const;
	int       fetchAdvanced(FetchVisitor* visitor) const;

	/**
	 * @brief getConn
	 * @param doNotConnect sometimes is OK to skip connection
	 * @return
	 */
	st_mysql* getConn(bool doNotConnect = false) const;
	ulong     lastId() const;

	// Non copyable
	DB& operator=(const DB&) = delete;
	DB(const DB&)            = delete;

	// this will require query + fetchAdvanced
	mutable mi_tls<bool> noFetch = false;
	// JUST For the next query the WARNING spam will be suppressed, use if you understand what you are doing
	// Reset itself in any case after 1 query
	mutable mi_tls<bool> skipWarning = false;

	const DBConf getConf() const;
	void         setConf(const DBConf& value);
	void         setConfIfNotSet(const DBConf& value);

	long getAffectedRows() const;
	//usually set / reset via
	//	ResetAfterUse resetMe(mainDB->state.get()
	//						  .NULL_as_EMPTY,
	//						  true);
	struct InternalState {
		uint    queryExecuted = 0;
		uint    reconnection  = 0;
		bool    NULL_as_EMPTY = false;
		QString lastError;
		quint64 totServerTime = 0;
		quint64 totFetchTime  = 0;
		qint64  serverTime    = 0;
		qint64  fetchTime     = 0;
		//In certain case we want to kill a running sql, is useless to emit a warning in that case, reset after USAGE
		//TODO change to be reset after use with the ResetAfter use class! (Or verify somehow, probably just easier to change type ?)(Or verify somehow, probably just easier to change type ?)
		bool skipNextDisconnect = false;
		//Reset after use, many times we DO NOT WANT an empty result cached, as will be populated very soon
		//TODO change to be reset after use with the ResetAfter use class! (Or verify somehow, probably just easier to change type ?)
		bool noCacheOnEmpty = false;
	};
	mutable mi_tls<InternalState> state;

      private:
	bool   confSet = false;
	DBConf conf;
	// Mutable is needed for all of them
	mutable mi_tls<long> affectedRows;
	// this allow to spam the DB handler around, and do not worry of thread, each thread will create it's own connection!
	mutable mi_tls<st_mysql*> connPool;
	// used for asyncs
	mutable mi_tls<int>        signalMask;
	mutable mi_tls<QByteArray> lastSQL;
	struct SharedState {
		std::atomic<uint> busyConnection = 0;
	};

	static SharedState sharedState;
};

using MYSQL_ROW = char**;
class FetchVisitor {
      public:
	virtual void processLine(MYSQL_ROW row)     = 0;
	virtual bool preCheck(st_mysql_res* result) = 0;
};

QString    QV(const sqlRow& line, const QByteArray& b);
QByteArray Q8(const sqlRow& line, const QByteArray& b);
QString    Q64(const sqlRow& line, const QByteArray& b);
quint64    getId(const sqlResult& res);

/**
 * @brief The SQLBuffering class it is a set of SQL queries with a flashing system which allows to execute
 * the queries (manually or automatically)
 */
class SQLBuffering {
      public:
	DB*         conn       = nullptr;
	uint        bufferSize = 1000;
	QStringList buffer;

	SQLBuffering() = default;
	/**
	 * @brief SQLBuffering
	 * @param _conn
	 * @param _bufferSize 0 disable auto flushing, 1 disable buffering
	 */
	SQLBuffering(DB* _conn, uint _bufferSize = 1000, bool _useTRX = true);
	~SQLBuffering();
	void append(const std::string& sql);
	void append(const QString& sql);
	void append(const QStringList& sqlList);
	void flush();
	void setUseTRX(bool _useTRX);
	void clear();

      private:
	// https://mariadb.com/kb/en/server-system-variables/#max_allowed_packet in our system is always 16M atm
	static const uint maxPacket = 16E6;
	// Set as false in case we are running inside another TRX
	bool useTRX = true;
};
