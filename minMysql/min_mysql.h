#pragma once

#include "DBConf.h"
#include "MITLS.h"
#include "mymaria.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/mapExtensor/mapV2.h"
#include "rbk/string/stringoso.h"
#include "sqlRow.h"
#include "sqlbuffering.h"
#include "sqlresult.h"
#include "sqlrowv2.h"
#include <QDateTime>
#include <QDebug>
#include <QStringList>

class DBException : public ExceptionV2 {
      public:
	//The mysql error code are, sight, a bunch of define -.- in mysqld_error.h
	enum Error : int {
		NA = 0,
		Warning,
		SchemaError,
		NoResult,
		InvalidDB  = 1049,
		DeadLock   = 1213,
		Duplicate  = 1062,
		Connection = 2013,
		//custom error code
		InvalidState = 9000

	} errorType = Error::NA;
	DBException(const StringAdt& _msg, Error error);
};

QString nullOnZero(uint v);

struct st_mysql;
struct st_mysql_res;

/**
 * @brief The MysqlW class is just to wrap the st_mysql to have RAII
 */
class St_mysqlW {
      public:
	operator st_mysql*();

	void set(st_mysql* c);

	~St_mysqlW();

	//just a cute counter
	inline static std::atomic_int connCounter = 0;

      private:
	st_mysql* conn = nullptr;
};

using StMysqlPtr = std::shared_ptr<St_mysqlW>;

QStringList getIdList(const sqlResult& sqlRes, const QString& idName);

class DB;
struct DBConf;

struct SQLLogger {
	SQLLogger(const std::string& _sql, bool _enabled, const DB* _db);
	void flush();
	~SQLLogger();

	qint64      serverTime;
	qint64      fetchTime;
	std::string sql;
	sqlResult   res;
	std::string error;
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
	void       closeConn() const;
	StMysqlPtr connect() const;
	bool       tryConnect() const;

	void setMaxQueryTime(uint time) const;

	/* Old Deprecated Method */
	sqlResult query(const StringAdt& sql) const;
	sqlRow    queryLine(const StringAdt& sql) const;

	SQLLogger queryInner(const std::string& sql) const;

	[[nodiscard]] sqlRow queryCacheLine2(const StringAdt& sql, uint ttl = 3600, bool required = false);

	[[nodiscard]] sqlResult queryCache2(const StringAdt& sql, const Opt& opt) const;
	[[nodiscard]] sqlResult queryCache2(const StringAdt& sql, uint ttl, bool required = false) const;

	//Try to read data from cache, if expired read from DB, if db unavailable use the cache, if all fail throw error
	[[nodiscard]] sqlResult queryORcache(const StringAdt& sql, uint ttl, bool required = false);

	// This is to be used ONLY in case the query can have deadlock, and internally tries multiple times to insert data
	[[nodiscard]] sqlResult queryDeadlockRepeater(const QByteArray& sql, uint maxTry = 5) const;

	/* V2 */
	[[nodiscard]] SqlResultV2 queryV2(const StringAdt& sql);
	[[nodiscard]] SqlResultV2 queryCacheV2(const StringAdt& sql, uint ttl);
	[[nodiscard]] SqlRowV2    queryCacheLineV2(const StringAdt& sql, uint ttl, bool required = false);

	void        pingCheck(st_mysql*& conn) const;
	QString     escape(const QString& what) const;
	std::string escape(const std::string& what) const;
	std::string escape(const std::string_view what) const;
	bool        isSSL() const;
	/**
	  Those 2 are used toghether for the ASYNC mode
	 * @brief startQuery
	 * @param sql
	 *
	 */
	void startQuery(const StringAdt& sql) const;

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

	sqlResult   fetchResult(SQLLogger* sqlLogger = nullptr) const;
	SqlResultV2 fetchResultV2(SQLLogger* sqlLogger = nullptr) const;

	u64 fetchAdvanced(FetchVisitor* visitor) const;

	/**
	 * @brief getConn
	 * @param doNotConnect sometimes is OK to skip connection
	 * @return
	 */
	st_mysql* getConn(bool doNotConnect = false) const;
	u64       lastId() const;
	u64       lastIdNoEx() const;

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
	//	#include "rbk/RAII/resetAfterUse.h"
	//	ResetOnExit resetMe(mainDB->state.get()
	//						  .NULL_as_EMPTY,
	//						  true);
	struct InternalState {
		std::string lastSQL;
		QString     lastError;
		qint64      totServerTime = 0;
		qint64      totFetchTime  = 0;
		qint64      serverTime    = 0;
		qint64      fetchTime     = 0;

		uint lastErrorCode = 0;
		uint queryExecuted = 0;
		uint reconnection  = 0;
		bool NULL_as_EMPTY = false;

		//In certain case we want to kill a running sql, is useless to emit a warning in that case, reset after USAGE
		//TODO change to be reset after use with the ResetAfter use class! (Or verify somehow, probably just easier to change type ?)(Or verify somehow, probably just easier to change type ?)
		bool skipNextDisconnect = false;

		//Reset after use, many times we DO NOT WANT an empty result cached, as will be populated very soon
		bool noCacheOnEmpty = false;
		//if we want to populate the result set with the TYPE of the column, rarely used, reset after use
		bool swapType = false;

		//does not automatically reset, hand with ResetOnExit
		bool uniqueViolationNothrow = false;
	};
	mutable mi_tls<InternalState> state;

      private:
	bool   confSet = false;
	DBConf conf;
	// Mutable is needed for all of them
	mutable mi_tls<u64> affectedRows;
	// this allow to spam the DB handler around, and do not worry of thread, each thread will create it's own connection!
	mutable mi_tls<StMysqlPtr> connPool;
	// used for asyncs
	mutable mi_tls<int> signalMask;
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
