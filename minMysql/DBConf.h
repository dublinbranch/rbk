#ifndef DBCONF_H
#define DBCONF_H

#include "rbk/QStacker/CxaLevel.h"
#include <QList>
#include <QString>
#include <memory>
#include <optional>

class QRegularExpression;
struct DBConf {
	DBConf();
	QByteArray                host = "127.0.0.1";
	QByteArray                pass;
	QByteArray                user;
	std::optional<QByteArray> sock;
	int64_t                   cacheId = 0;
	std::optional<bool>       ssl     = false;
	// Usually set false for operation that do not have to be replicated
	bool writeBinlog = true;
	//sometimes we connect with low privileges or weird limit like in GCP or orrible bug
	std::optional<bool> isMariaDB8 = false;

	// This header is quite big, better avoid the inclusion
	std::vector<std::shared_ptr<QRegularExpression>> warningSuppression;

	//This is dangerous water, a short timeout will trigger error on long sql, but also will not be responsive on short one
	//yes is a fault in mysql protocol that should have some kind of heartbeat mechanism while the sql is ongoing
	//the default of 0 means never timeout, which... is not the best
	std::optional<uint> readTimeout     = 0;
	std::optional<uint> port            = 3306;
	std::optional<bool> logSql          = false;
	std::optional<bool> logError        = false;
	bool                pingBeforeQuery = true; // So if the connection is broken will be re-established
	//we normally work in local
	bool                compress        = false;

	//Old compatibility logic for old code, we now normally alwys use TRUE
	bool NULL_as_EMPTY = false;
	// In certain case not beeing able to connect is bad, in other not and we just go ahead, retry later...
	CxaLevel connErrorVerbosity = CxaLevel::none;

	// Corpus munus
	QByteArray getDefaultDB() const;
	void       setDefaultDB(const QByteArray& value);
	QString    getInfo(bool passwd = false) const;
	void       setWarningSuppression(std::vector<QString> regex);

	QByteArray defaultDB;

      private:
};

#endif // DBCONF_H
