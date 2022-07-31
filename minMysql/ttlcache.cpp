#include "ttlcache.h"

TTLCache::TTLCache(const DBConf& conf) {
	setConf(conf);
}

void TTLCache::setConf(const DBConf& conf) {
	db.setConf(conf);
	//this will check if we have the proper table and column available in the selected DB
	try {
		auto row = db.queryLine("SELECT id, key, ttl FROM ttlcache ORDER BY id DESC LIMIT 1");
	} catch (QString e) {
		QString msg = R"(
The DB is probably missing the ttlcache table in the db %1, create it with
CREATE TABLE `runnable` (
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`operationCode` varchar(65000) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
	`lastRun` int(10) unsigned NOT NULL,
	`orario` datetime GENERATED ALWAYS AS (from_unixtime(`lastRun`)) VIRTUAL,
	PRIMARY KEY (`id`),
KEY `lastRun` (`lastRun`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
					  )";
		msg         = msg.arg(QString(db.getConf().getDefaultDB()));
		throw DBException(msg, DBException::Error::SchemaError);
	}
}

QByteArray TTLCache::get(const QString& key) const {
	(void)key;
	return QByteArray();
}

bool TTLCache::set(const QString& key, uint ttl, const QByteArray& payload) {
	(void)key;
	(void)ttl;
	(void)payload;
	return true;
}
