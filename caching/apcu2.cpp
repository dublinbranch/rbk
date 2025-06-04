#include "apcu2.h"
#include "unistd.h"

#include <cstdlib>
#include <fmt/color.h>
#include <mutex>
#include <thread>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include "rbk/caching/cachable.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/serialization/QDataStreamer.h"

#include <QDataStream>
#include <QSaveFile>
#include <rbk/mapExtensor/qmapV2.h>

using namespace std;

struct ByExpire {};
struct ByKey {};
struct ByTypeHashCode {};

using boost::multi_index_container;
using namespace boost::multi_index;

struct ApcuCache_index : indexed_by<
                             hashed_unique<
                                 tag<ByKey>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, std::string, key)>,
                             hashed_non_unique<
                                 tag<ByTypeHashCode>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, size_t, typeHashCode)>,
                             ordered_non_unique<
                                 tag<ByExpire>, BOOST_MULTI_INDEX_MEMBER(APCU::Row, i64, expireAt)>> {};

using ApcuCache = multi_index_container<APCU::Row, ApcuCache_index>;
//this is deallocated at exit before the function is called, so we just manually manage it, no idea how to do the "correct" way
static ApcuCache* cache = nullptr;

//Orrible, as will trigger immediately the GC thread creation ecc, those must be delayed, and most important enabled AFTER main if needed in the project...
static APCU theAPCU;
using DiskMapType = QMapV2<std::string, APCU::DiskValue>;

static const int version = 4;

void diskSync() {
	if (APCU::disableAPCU) {
		return;
	}
	auto a = APCU::getInstance();
	a->diskSyncP1();
}

APCU::APCU()
    : startedAt(QDateTime::currentSecsSinceEpoch()) {
	if (disableAPCU) {
		return;
	}

	cache = new ApcuCache();
	diskLoad();
	std::thread apt(&APCU::garbageCollector_F2, this);
	apt.detach();
	std::atexit(diskSync);
}

APCU* APCU::getInstance() {
	return &theAPCU;
}

any* APCU::fetchInner(const std::string& key) {
	auto& byKey = cache->get<ByKey>();

	std::shared_lock lock(innerLock);

	if (auto iter = byKey.find(key); iter != cache->end()) {

		hits++;
		auto* value = const_cast<std::any*>(&iter->value);
		//fmt::print("Debug - has_value: {}, type: {}\n", value->has_value(), value->type().name());
		return value;
	}
	miss++;
	static std::any empty;
	return &empty;
}

void APCU::storeInner(const Row& row, bool overwrite_) {
	//TODO if is persistent, check if a qbytearray too else the write on disk will fail and is lost!

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

void APCU::remove(const std::string& key) {
	auto&            byKey = cache->get<ByKey>();
	std::unique_lock lock(innerLock);
	if (byKey.erase(key)) {
		deleted++;
	}
}

std::string APCU::info() const {
	//Poor man APCU page -.-
	double delta = (double)(QDateTime::currentSecsSinceEpoch() - startedAt);
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
	                           cache->size(), hits.load(), (double)hits / delta, miss.load(), (double)miss / delta, // 5
	                           insert.load(), (double)insert / delta, overwrite.load(), (double)overwrite / delta, deleted.load(), (double)deleted / delta);
	return msg;
}

//only used internally
void throwTypeError(const type_info* found, const type_info* expected) {
	throw ExceptionV2(QSL("Wrong type!! Found %1, expected %2, recheck where this key is used, maybe you have a collision").arg(found->name()).arg(expected->name()));
}

void APCU::Row::setTTL(i64 ttl) {
	expireAt = QDateTime::currentSecsSinceEpoch() + ttl;
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
	//fmt::print("Start collecting data to write on disk\n");
	DiskMapType      toBeWritten;
	std::shared_lock lock(innerLock);

	auto& byKey = cache->get<ByKey>();
	for (auto& iter : byKey) {
		if (!iter.persistent) {
			continue;
		}
		{
			auto el = any_cast<QByteArray>(&iter.value);
			if (el) {
				toBeWritten.insert(iter.key, {static_cast<uint>(iter.expireAt), iter.typeHashCode, *el});
				continue;
			}
		}
		{
			auto el = any_cast<std::shared_ptr<Cachable>>(&iter.value);
			if (el) {
				toBeWritten.insert(iter.key, {static_cast<uint>(iter.expireAt), iter.typeHashCode, el->get()->serialize()});
			}
		}
	}

	//fmt::print("{} element to write\n", toBeWritten.size());

	QSaveFile file("apcu.dat");
	if (!file.open(QIODevice::WriteOnly)) {
		qCritical("impossible to save apcu cache!");
	}
	QDataStream out(&file);

	out << version;
	out << toBeWritten;
	file.commit();
	//fmt::print("{} byte wrote\n", file.size());
}

void APCU::diskSyncP1() {
	//stop garbageCollector_F2

	//send stop signal
	requestGarbageCollectorStop.test_and_set();

	//wait for the thread to exit
	garbageCollectorRunning.wait(true);

	diskSyncP2();
}

void APCU::diskLoad() {
	QFile file("apcu.dat");
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	QDataStream in(&file);
	int         versionDisk = 0;
	in >> versionDisk;
	if (version != versionDisk) {
		auto str = fmt::format(fg(fmt::color::red), "APCU version outdated! Remove the apcu.dat file to continue on disk {} expected {}\n", versionDisk, version);
		fmt::print(stderr, "{}", str);
		exit(1);
	}
	DiskMapType toBeWritten;

	std::shared_lock lock(innerLock);
	in >> toBeWritten;

	fmt::print("APCU found {} element to reload\n", toBeWritten.size());

	for (auto&& [key, line] : toBeWritten) {
		APCU::Row row;
		row.key          = key;
		row.typeHashCode = line.typeHashCode;
		row.value        = line.value;
		row.expireAt     = line.expireAt;
		//ofc if you reload something from disk it was born persistent!
		row.persistent = true;
		//fmt::print("reloaded {} expire @ {} \n", key, line.expireAt);
		cache->emplace(row);
	}
}

void APCU::garbageCollector_F2() {
	if (disableAPCU) {
		return;
	}
	//set thread name
	pthread_setname_np(pthread_self(), "APCU GC");
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

			//so we skip expire = 0 (disable)
			auto iter = byExpire.lower_bound(1);
			//we limit the search to thing in the past
			auto max = byExpire.upper_bound(now);

			while (iter != max) {
				auto& row = *iter;
				(void)row;
				iter = byExpire.erase(iter);
				deleted++;
			}
		}
		if (diskSaveTime) {
			static qint64 lastSync = 0;
			if (now > lastSync + diskSaveTime) {
				diskSyncP2();
				lastSync = now;
			}
		}
	}
	garbageCollectorRunning.clear();
	garbageCollectorRunning.notify_one();
}

std::vector<std::any*> APCU::getAllByTypeInner(size_t hashCode) {
	auto&            key = cache->get<ByTypeHashCode>();
	std::shared_lock lock(innerLock);

	std::vector<std::any*> result;
	auto                   range = key.equal_range(hashCode);
	for (auto it = range.first; it != range.second; ++it) {
		auto el = const_cast<std::any*>(&it->value);
		result.push_back(el);
	}
	return result;
}

void apcuStore(const APCU::Row& row) {
	auto a = APCU::getInstance();
	a->storeInner(row, true);
}

FetchPodResult fetchPOD(const std::string& key) {
	auto a   = APCU::getInstance();
	auto res = a->fetchInner(key);

	// fmt::print("Debug fetchPOD - res: {}, has_value: {}, type: {}\n",
	//            (void*)res, res->has_value(), res->type().name());

	if (res->has_value()) {
		return {any_cast<QByteArray>(*res), true};
	}
	return {{}, false};
}

FetchPodResult fetchPOD(const QString& key) {
	return fetchPOD(key.toStdString());
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QDataStream& operator<<(QDataStream& out, const APCU::DiskValue& v) {
	out << v.expireAt;
	out << v.value;
	return out;
}

QDataStream& operator>>(QDataStream& in, APCU::DiskValue& v) {
	in >> v.expireAt;
	in >> v.value;
	return in;
}
#endif

void apcuRemove(const StringAdt& key) {
	auto a = APCU::getInstance();
	a->remove(key);
}

void storePOD(const std::string& key, const QByteArray& value, uint ttl, bool persistent) {
	APCU::Row row;
	row.key = key;
	row.setPOD(value);
	row.expireAt   = QDateTime::currentSecsSinceEpoch() + ttl;
	row.persistent = persistent;
	apcuStore(row);
}
