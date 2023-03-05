#include "runnable.h"
#include "min_mysql.h"
#include "rbk/misc/b64.h"

Runnable::Runnable(DB* db_) {
	setConf(db_);
}

void Runnable::setConf(DB* db_) {
	db = db_;
	// this will check if we have the proper table and column available in the selected DB
	try {
		auto row = db_->queryLine("SELECT id, operationCode FROM runnable.runnable ORDER BY lastRun DESC LIMIT 1");
	} catch (std::exception& e) {
		auto msg = R"(
Is probably missing the runnable table in the db runnable, create it with
CREATE DATABASE runnable;
CREATE TABLE runnable.`runnable` (
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`operationCode` varchar(65000) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
	`lastRun` int(10) unsigned NOT NULL,
	`orario` datetime GENERATED ALWAYS AS (from_unixtime(`lastRun`)) VIRTUAL,
	PRIMARY KEY (`id`),
KEY `lastRun` (`lastRun`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
					  )";

		qCritical().noquote() << e.what() << msg;
		exit(1);
	}
}

bool Runnable::runnable(const QString& key, qint64 second) {
	if (forceRunnable) {
		return true;
	}

	static const QString skel = "SELECT id, lastRun FROM runnable.runnable WHERE operationCode = %1 ORDER BY lastRun DESC LIMIT 1";
	auto                 now  = QDateTime::currentSecsSinceEpoch();
	auto                 sql  = skel.arg(base64this(key));
	auto                 res  = db->query(sql);
	if (res.isEmpty() or res.at(0).value("lastRun", BZero).toLongLong() + second < now) {
		static const QString insertSkel = "INSERT INTO runnable.runnable SET operationCode = %1, lastRun = %2";
		auto                 insertSql  = insertSkel.arg(base64this(key)).arg(now);
		db->query(insertSql);

		return true;
	} else {
		return false;
	}
}

bool Runnable::operator()(const QString& key, qint64 second) {
	return runnable(key, second);
}
