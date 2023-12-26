#pragma once

#include "rbk/QStacker/exceptionv2.h"
#include "rbk/misc/intTypes.h"
#include "rbk/mixin/NoCopy.h"
#include <QDateTime>
#include <QString>
#include <any>
#include <memory>
#include <shared_mutex>

#define QSL(str) QStringLiteral(str)
void throwTypeError(const std::type_info* found, const std::type_info* expected);

class APCU : private NoCopy {
      public:
	APCU();
	static APCU* getInstance();

	//to disable
	/* Apply the constructor attribute to startupfun() so that it is executed before main()
	   void startupfun (void) __attribute__ ((constructor));

	   void startupfun (void)
	   {   APCU::disableAPCU = true;    }
*/

	static inline bool disableAPCU = false;

	struct Row {
		//Corpus munus
		Row() = default;
		template <typename T>
		[[nodiscard]] Row(const std::string& key_, const T& value_, u64 expireAt_) {
			*this = Row(key_, std::make_shared<T>(value_), expireAt_);
		}

		template <typename T>
		[[nodiscard]] Row(const std::string& key_, const std::shared_ptr<T>& value_, u64 expireAt_) {
			*this = Row(key_, std::any(value_), expireAt_);
		}

		[[nodiscard]] Row(const std::string& key_, const std::any& value_, u64 expireAt_);

		void set();

		//Member
		std::string key;
		std::any    value;
		//0 will disable flushing
		i64 expireAt = 1;
		//Only QByteArray is accepted for that kind, the cached type will provide the
		//serialized / unserialize operation
		//those key will be saved on disk on program CLOSE and reloaded, they still have the same TTL based logic
		bool persistent = false;

		bool expired() const;
		bool expired(qint64 ts) const;
	};

	/**
	 * We hide the implementation as multi index will kill compile time
	 */
	std::any fetchInner(const std::string& key);
	void     storeInner(const std::string& _key, const std::any& _value, bool overwrite_ = false, u64 ttl = 60);
	void     storeInner(const APCU::Row& row_, bool overwrite_);

	template <class T>
	std::shared_ptr<T> fetch(const std::string& key) {
		(void)key;
		auto res = fetchInner(key);
		if (res.has_value()) {
			//auto& type = res.type();
			//auto  name = type.name();
			return any_cast<std::shared_ptr<T>>(res);
		} else {
			return nullptr;
		}
	}

	[[nodiscard]] std::string info() const;

	/**
	 * @brief store will OVERWRITE IF IS FOUND
	 * @param key
	 * @param obj
	 * @param ttl
	 */
	template <class T>
	void store(const std::string& key, std::shared_ptr<T>& obj, int ttl = 60) {
		std::any value = obj;
		storeInner(key, value, true, ttl);
	}

	void clear();

	struct DiskValue {
		quint32    expireAt = 1;
		QByteArray value;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

		friend QDataStream& operator<<(QDataStream& out, const DiskValue& v) {
			out << v.expireAt;
			out << v.value;
			return out;
		}

		friend QDataStream& operator>>(QDataStream& in, DiskValue& v) {
			//for retarded reason in qt5 this line fails -.- saying it can not find the right conversion
			in >> v.expireAt;
			in >> v.value;
			return in;
		}
#endif
	};

	void diskSyncP2();
	void diskSyncP1();
	void diskLoad();

	//1 overwrite will NOT trigger 1 delete and 1 inserted
	std::atomic<uint64_t> overwrite;
	std::atomic<uint64_t> insert;
	std::atomic<uint64_t> deleted;
	std::atomic<uint64_t> hits;
	std::atomic<uint64_t> miss;

      private:
	void              garbageCollector_F2();
	std::shared_mutex innerLock;
	qint64            startedAt = 0;
	std::atomic_flag  garbageCollectorRunning;
	std::atomic_flag  requestGarbageCollectorStop = false;

	//	/**
	//	 * @brief apcuTryStore
	//	 * @param key
	//	 * @param obj
	//	 * @param ttl
	//	 * @return if we inserted (there was NONE or EXPIRED) or not
	//	 */
	//	template <class T>
	//	bool tryStore(const QString& key, T& obj, int ttl = 60) {
	//		CacheType::iterator         iter;
	//		std::lock_guard<std::mutex> scoped(innerLock);
	//		if (exists(key, false, iter)) {
	//			return true;
	//		} else {
	//			iter->second = Value(obj, ttl);
	//			return false;
	//		}
	//	}
};

void apcuStore(const APCU::Row& row);

struct FetchPodResult {
	QByteArray value;
	bool       found;

	explicit operator bool() const {
		return found;
	}
};

FetchPodResult fetchPOD(const QString& key);
FetchPodResult fetchPOD(const std::string& key);

template <class T>
void apcuStore(const std::string& key, std::shared_ptr<T>& obj, int ttl = 60) {
	auto a = APCU::getInstance();
	a->store(key, obj, ttl);
}

template <class T>
void apcuStore(const QString& key, std::shared_ptr<T>& obj, int ttl = 60) {
	apcuStore(key.toStdString(), obj, ttl);
}

template <class T>
void apcuStore(const std::string& key, const T& obj, int ttl = 60) {
	auto copy = std::make_shared<T>(obj);
	apcuStore(key, copy, ttl);
}

template <class T>
void apcuStore(const QString& key, const T& obj, int ttl = 60) {
	apcuStore(key.toStdString(), obj, ttl);
}

template <class T>
std::shared_ptr<T> apcuFetch(const std::string& key) {
	auto a   = APCU::getInstance();
	auto res = a->fetch<T>(key);
	return res;
}

template <class T>
std::shared_ptr<T> apcuFetch(const QString& key) {
	return apcuFetch<T>(key.toStdString());
}

void apcuClear();
int  apcuTest();
