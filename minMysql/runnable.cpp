#include "runnable.h"
#include "min_mysql.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/b64.h"
#include <iostream>

Runnable runnable;

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
	`coolDown` int(10) unsigned NOT NULL DEFAULT 0,
	`orario` datetime GENERATED ALWAYS AS (from_unixtime(`lastRun`)) VIRTUAL,
	PRIMARY KEY (`id`),
KEY `lastRun` (`lastRun`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
					  )";

		std::cerr << e.what() << msg;
		exit(1);
	}
}

bool Runnable::runnable_64(const QString& key, i64 second, double multiplier) {
	return runnable(key, second, multiplier);
}

bool Runnable::runnable(const QStringAdt& key, i64 second, double multiplier) {
	if (forceRunnable) {
		return true;
	}

	static const QString skel = "SELECT id, lastRun, coolDown FROM runnable.runnable WHERE operationCode = %1 ORDER BY lastRun DESC LIMIT 1";
	auto                 now  = QDateTime::currentSecsSinceEpoch();
	auto                 sql  = skel.arg(base64this(key));
	auto                 row  = db->queryLine(sql);

	auto insertSql = F16("INSERT INTO runnable.runnable SET operationCode = {}, lastRun = {}, coolDown = {}",
	                     base64this(key), now, second);

	if (row.isEmpty()) {
		db->query(insertSql);
		return true;
	}
	auto lastRun  = row.rq<i64>("lastRun");
	auto coolDown = second;

	//if we are using the cooldown scaling take into account the last one
	if (multiplier != 1) {
		row.rq("coolDown", coolDown);
		coolDown = static_cast<i64>(static_cast<double>(coolDown) * multiplier);
	}

	if (lastRun + coolDown > now) {
		return false;
	}

	db->query(insertSql);
	return true;
}

bool Runnable::operator()(const QStringAdt &key, i64 second, double multiplier) {
	return runnable(key, second, multiplier);
}
