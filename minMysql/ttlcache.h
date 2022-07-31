#ifndef TTLCACHE_H
#define TTLCACHE_H

#include "min_mysql.h"
class TTLCache {
      public:
	TTLCache(const DBConf& conf);
	TTLCache() = default;

	void setConf(const DBConf& conf);

	QByteArray get(const QString& key) const;
	bool       set(const QString& key, uint ttl, const QByteArray& payload);
	//Non copyable
	TTLCache& operator=(const TTLCache&) = delete;
	TTLCache(const TTLCache&)            = delete;

      private:
	DB db;
};

#endif // TTLCACHE_H
