#ifndef HOME_ROY_PUBLIC_DITER_CLASS_LOG_H
#define HOME_ROY_PUBLIC_DITER_CLASS_LOG_H

#include "rbk/BoostJson/fwd.h"
#include "rbk/minMysql/sqlbuffering.h"
#include "rbk/string/stringoso.h"
#include <QElapsedTimer>
#include <QString>
#include <qdatetime.h>
#include <vector>

#include "rbk/mixin/NoCopy.h"

class Log;
using Logs = std::list<Log>;

class Log : public std::exception {
      public:
	//An EXTREMELY invasive check that all usefull paramer are set
	static inline bool developMode = false;

	enum Category {
		notSet,
		//A log that is not an Error is considered Ok
		Info,
		Warning,
		Error,
		//We use this level at the entrance of function that might have exception, so we know if one happened
		Exception
	} category = notSet;
	QDateTime tsStart;
	qint64    elapsed = 0;

	//This can either be the function name or the section of the code or the actual call, it depends on the context
	QString     section;
	std::string options;

	//the actual message
	QByteArray stdOut;
	QByteArray stdErr;
	QByteArray stackTrace;

	//many times we want to aggregate log for a specific function or process execution
	Logs subLogs;

	[[nodiscard]] bool hasError(bool recursive = false) const;

	[[nodiscard]] std::string serialize();
	[[nodiscard]] QString     serialize(QString);
	boost::json::object       toJson();
	bj::object                toJson4panel() const;

	SQLBuffering toSqlRow() const;

	Log();
	Log(const QByteArray& _info, Category _category = Info);
	Log(const std::exception& e, const char* func);

	~Log();
	bool used = false;
	void push(Log&& log);
	void push(Log& log);

	void setEnd();
	void setStdErr(const QByteAdt v);

      private:
	QElapsedTimer timer;
};

#endif // HOME_ROY_PUBLIC_DITER_CLASS_LOG_H
