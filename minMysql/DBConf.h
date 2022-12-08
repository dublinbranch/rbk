#ifndef DBCONF_H
#define DBCONF_H

#include "rbk/QStacker/CxaLevel.h"
#include <QList>
#include <QString>
#include <memory>

class QRegularExpression;
struct DBConf {
	DBConf();
	QByteArray host = "127.0.0.1";
	QByteArray pass;
	QByteArray user;
	QByteArray sock;
	int64_t    cacheId = 0;
	bool       ssl     = false;
	// Usually set false for operation that do not have to be replicated
	bool writeBinlog = true;
	//sometimes we connect with low privileges or weird limit like in GCP
	bool noSqlMode = false;
	// This header is quite big, better avoid the inclusion
	std::vector<std::shared_ptr<QRegularExpression>> warningSuppression;

	uint readTimeout     = 0;
	uint port            = 3306;
	bool logSql          = false;
	bool logError        = false;
	bool pingBeforeQuery = true; // So if the connection is broken will be re-established
	// In certain case not beeing able to connect is bad, in other not and we just go ahead, retry later...
	CxaLevel connErrorVerbosity = CxaLevel::none;

	// Corpus munus
	QByteArray getDefaultDB() const;
	void       setDefaultDB(const QByteArray& value);
	QString    getInfo(bool passwd = false) const;
	void       setWarningSuppression(std::vector<QString> regex);

      private:
	QByteArray defaultDB;
};

#endif // DBCONF_H
