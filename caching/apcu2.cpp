#include "apcu2.h"
#include "unistd.h"

#include <cstdlib>
#include <mutex>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include "rbk/filesystem/filefunction.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/serialization/QDataStreamer.h"

#include <QDataStream>
#include <rbk/mapExtensor/qmapV2.h>

using namespace std;

struct ByExpire {};
struct ByKey {};

using boost::multi_index_container;
using namespace boost::multi_index;

struct ApcuCache_index : indexed_by<
                             hashed_unique<
                                 tag<ByKey>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, std::string, key)>,
                             ordered_non_unique<
                                 tag<ByExpire>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, uint, expireAt)>> {};

using ApcuCache = multi_index_container<APCU::Row, ApcuCache_index>;
//this is deallocated at exit before the function is called, so we just manually manage it, no idea how to do the "correct" way
static ApcuCache* cache = nullptr;

static APCU theAPCU;
using DiskMapType = QMapV2<std::string, APCU::DiskValue>;

void diskSync() {
	auto a = APCU::getInstance();
	a->diskSyncP1();
}

APCU::APCU()
    : startedAt(QDateTime::currentSecsSinceEpoch()) {

	cache = new ApcuCache();
	diskLoad();
	new std::thread(&APCU::garbageCollector_F2, this);
	std::atexit(diskSync);
}

APCU* APCU::getInstance() {
	return &theAPCU;
}

std::any APCU::fetchInner(const std::string& key) {
	auto& byKey = cache->get<ByKey>();

	std::shared_lock lock(innerLock);

	if (auto iter = byKey.find(key); iter != cache->end()) {

		hits++;
		return iter->value;

		//unlock and just relock is bad, as will leave a GAP!
		//you should unlock, restart the operation under full lock, and than erase...
		//who cares, in a few second the GC will remove the record anyways
	}
	miss++;
	return {};
}

void APCU::storeInner(const std::string& _key, const std::any& _value, bool overwrite_, int ttl) {
	uint expireAt = 0;
	if (ttl) {
		expireAt = QDateTime::currentSecsSinceEpoch() + ttl;
	}
	Row row(_key, _value, expireAt);
	storeInner(row, overwrite_);
}

void APCU::storeInner(const Row& row, bool overwrite_) {
	auto& byKey = cache->get<ByKey>();

	std::unique_lock lock(innerLock);

	if (auto iter = byKey.find(row.key); iter != cache->end()) {
		if (overwrite_) {
			overwrite++;

			byKey.replace(iter, row);
		}
	} else {
		insert++;
		cache->insert(row);
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
	                           cache->size(), hits, hits / delta, miss, miss / delta, // 5
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

void APCU::diskSyncP2() {
	fmt::print("Start collecting data to write on disk\n");
	DiskMapType      toBeWritten;
	std::shared_lock lock(innerLock);

	auto& byKey = cache->get<ByKey>();
	for (auto& iter : byKey) {
		if (!iter.persistent) {
			continue;
		}
		try {
			auto copy = any_cast<QByteArray>(iter.value);
			toBeWritten.insert(iter.key, {iter.expireAt, copy});
		} catch (std::bad_any_cast& e) {
			(void)e;
			fmt::print("key {} is not a QByteArray! \n", iter.key);
		}
	}

	fmt::print("{} element to write\n", toBeWritten.size());

	QFile file("apcu.dat");
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);
	out << toBeWritten;
	file.flush();
	fmt::print("{} byte wrote\n", file.size());
}

void APCU::diskSyncP1() {
	//stop garbageCollector_F2

	requestGarbageCollectorStop.test_and_set();
	garbageCollectorRunning.wait(true);

	diskSyncP2();
}

void APCU::diskLoad() {
	QFile file("apcu.dat");
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	QDataStream in(&file);
	DiskMapType toBeWritten;

	std::shared_lock lock(innerLock);
	in >> toBeWritten;

	fmt::print("APCU found {} element to reload\n", toBeWritten.size());

	for (auto&& [key, line] : toBeWritten) {
		APCU::Row row(key, line.value, line.expireAt);
		//ofc if you reload something from disk it was born persistent!
		row.persistent = true;
		cache->emplace(row);
	}
}

void APCU::garbageCollector_F2() {
	//TODO On program exit stop this gc operation, else we will have double free problem
	auto& byExpire = cache->get<ByExpire>();
	garbageCollectorRunning.test_and_set();
	while (true) {
		if (requestGarbageCollectorStop.test()) {
			break;
		}
		sleep(1);
		if (requestGarbageCollectorStop.test()) {
			break;
		}
		auto now = QDateTime::currentSecsSinceEpoch();
		{
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
		//WARNING FIXME REMOVE ME ONLY FOR DEBUG!
		//diskSyncP2();
	}
	garbageCollectorRunning.clear();
	garbageCollectorRunning.notify_one();
}

void apcuStore(const APCU::Row& row) {
	auto a = APCU::getInstance();
	a->storeInner(row, true);
}

FetchPodResult fetchPOD(const std::string& key) {
	auto a   = APCU::getInstance();
	auto res = a->fetchInner(key);
	if (res.has_value()) {
		return {any_cast<QByteArray>(res), true};
	}
	return {{}, false};
}

FetchPodResult fetchPOD(const QString& key) {
	return fetchPOD(key.toStdString());
}

QDataStream& operator<<(QDataStream& out, const APCU::DiskValue& rhs) {
	out << rhs.expireAt;
	out << rhs.value;
	return out;
}

QDataStream& operator>>(QDataStream& in, APCU::DiskValue& rhs) {
	in >> rhs.expireAt;
	in >> rhs.value;
	return in;
}
