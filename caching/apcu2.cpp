#include "apcu2.h"
#include "unistd.h"

#include <mutex>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include "rbk/fmtExtra/includeMe.h"

using namespace std;
static APCU theAPCU;

struct ByExpire {};
struct ByKey {};

using boost::multi_index_container;
using namespace boost::multi_index;

struct ApcuCache_index : indexed_by<
                             hashed_unique<
                                 tag<ByKey>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, std::string, key)>,
                             ordered_non_unique<
                                 tag<ByExpire>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, uint, expireAt)>> {};

typedef multi_index_container<APCU::Row, ApcuCache_index> ApcuCache;
ApcuCache                                                 cache;
APCU::APCU() {
	startedAt = QDateTime::currentSecsSinceEpoch();
	new std::thread(&APCU::garbageCollector_F2, this);
}

APCU* APCU::getInstance() {
	return &theAPCU;
}

std::any APCU::fetchInner(const std::string& key) {
	auto& byKey = cache.get<ByKey>();

	std::shared_lock lock(innerLock);

	if (auto iter = byKey.find(key); iter != cache.end()) {

		hits++;
		return iter->value;

		//unlock and just relock is bad, as will leave a GAP!
		//you should unlock, restart the operation under full lock, and than erase...
		//who cares, in a few second the GC will remove the record anyways
	}
	miss++;
	return {};
}

void APCU::storeInner(const std::string& _key, const std::any& _value, bool _overwrite, int ttl) {
	auto& byKey = cache.get<ByKey>();

	std::unique_lock lock(innerLock);
	uint             expireAt = 0;
	if (ttl) {
		expireAt = QDateTime::currentSecsSinceEpoch() + ttl;
	}

	if (auto iter = byKey.find(_key); iter != cache.end()) {
		if (_overwrite) {
			overwrite++;

			byKey.replace(iter, Row(_key, _value, expireAt));
		}
	} else {
		insert++;
		cache.emplace(Row(_key, _value, expireAt));
	}
}

std::string APCU::info() const {
	//Poor man APCU page -.-
	double delta = QDateTime::currentSecsSinceEpoch() - startedAt;
	auto   msg   = fmt::format(R"(
<pre>
		Cache size: {:>10}
		Hits:       {:>10} / {:>8.0f}s
		Miss:       {:>10} / {:>8.0f}s
		Insert:     {:>10} / {:>8.0f}s
		Overwrite:  {:>10} / {:>8.0f}s
		Delete:     {:>10} / {:>8.0f}s
</pre>
		)",
	                           cache.size(), hits, hits / delta, miss, miss / delta, // 5
	                           insert, insert / delta, overwrite, overwrite / delta, deleted, deleted / delta);
	return msg;
}

//only used internally
void throwTypeError(const type_info* found, const type_info* expected) {
	throw ExceptionV2(QSL("Wrong type!! Found %1, expected %2, recheck where this key is used, maybe you have a collision").arg(found->name()).arg(expected->name()));
}

APCU::Row::Row(const std::string& key_, const std::any& value_, int expireAt_) {
	key      = key_;
	value    = value_;
	expireAt = expireAt_;
}

bool APCU::Row::expired() const {
	return QDateTime::currentSecsSinceEpoch() > expireAt;
}

bool APCU::Row::expired(qint64 ts) const {
	if (expireAt) {
		return ts > expireAt;
	}
	return false;
}

void APCU::garbageCollector_F2() {
	//TODO On program exit stop this gc operation, else we will have double free problem
	auto& byExpire = cache.get<ByExpire>();
	while (true) {
		sleep(1);
		auto now = QDateTime::currentSecsSinceEpoch();

		std::unique_lock lock(innerLock);

		auto upper = byExpire.upper_bound(now);
		//so we skip expire = 0
		auto iter = byExpire.lower_bound(1);

		while (true) {
			//You can not have an OR condition in the for ?
			auto b = iter == byExpire.end();
			auto c = iter == upper;
			if (b || c) {
				break;
			}
			auto& row = *iter;
			(void)row;
			if (iter->expired(now)) {
				iter = byExpire.erase(iter);
				deleted++;
				continue;
			}
			iter++;
		}
	}
}
