#ifndef HOME_ROY_PUBLIC_DITER_CLASS_LOG_H
#define HOME_ROY_PUBLIC_DITER_CLASS_LOG_H

#include "rbk/minMysql/sqlbuffering.h"
#include "rbk/string/stringoso.h"
#include <QElapsedTimer>
#include <QString>
#include <qdatetime.h>
#include <vector>

class Log;
using Logs = std::vector<Log>;

class Log {
      public:
	enum Category {
		Info,
		Warning,
		Error,
		Exception
	} category;
	QDateTime tsStart;
	qint64    elapsed = 0;

	//This can either be the function name or the section of the code or the actual call, it depends on the context
	QString section;

	//the actual message
	QByteArray   stdOut;
	QByteArrayV2 stdErr;
	QByteArray   stackTrace;

	//many times we want to aggregate log for a specific function or process execution
	Logs subLogs;

	QString      serialize();
	SQLBuffering toSqlRow() const;

	Log();
	Log(const QByteArray& _info, Category _category = Info);
	Log(const std::exception& e, const char* func);

	void push(const Log& log);

	void setEnd();

      private:
	QElapsedTimer timer;
};

#endif // HOME_ROY_PUBLIC_DITER_CLASS_LOG_H
