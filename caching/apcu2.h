#pragma once

#include "rbk/QStacker/exceptionv2.h"
#include "rbk/concept/isSharedPtr.h"
#include "rbk/misc/intTypes.h"
#include "rbk/mixin/NoCopy.h"
#include "rbk/string/stringoso.h"
#include <QDateTime>
#include <QString>
#include <any>
#include <concepts>
#include <memory>
#include <shared_mutex>

#define QSL(str) QStringLiteral(str)
void throwTypeError(const std::type_info* found, const std::type_info* expected);

class Cachable;

class APCU : private NoCopy {
      public:
	APCU();
	static APCU* getInstance();

	//to disable
	/* Apply the constructor attribute to startupfun() so that it is executed before main()
	 * thos must be two separated entity
	 
	   void startupfun (void) __attribute__ ((constructor));
    
	   void startupfun (void) 
	   {   APCU::disableAPCU = true;    }
*/

	static inline bool disableAPCU = false;
	//if 0 disabled, else every how many second to save on disk
	static inline uint diskSaveTime = 0;

	struct Row {
		friend class APCU;
		//Corpus munus
		Row() = default;

		// template <typename T>
		// [[nodiscard]] Row(const std::string& key_, const T& value_, u64 expireAt_) {
		// 	*this = Row(key_, std::make_shared<T>(value_), expireAt_);
		// }

		// template <typename T>
		// [[nodiscard]] Row(const std::string& key_, const std::shared_ptr<T>& value_, u64 expireAt_) {
		// 	*this = Row(key_, std::any(value_), expireAt_);
		// }

		// [[nodiscard]] Row(const std::string& key_, const std::any& value_, u64 expireAt_);

		void set();

		//Member
		std::string key;
		size_t      typeHashCode = 0;

		//0 will disable flushing
		i64 expireAt = 1;
		//Only QByteArray is accepted for that kind, the cached type will provide the
		//serialized / unserialize operation
		//those key will be saved on disk on program CLOSE and reloaded, they still have the same TTL based logic
		bool persistent = false;

		void setTTL(i64 ttl);
		bool expired() const;
		bool expired(qint64 ts) const;

	      public:
		void setPOD(const QByteArray& value_) {
			value = value_;
		}
		//TODO create another function that will copy the object

		/**
		 * @brief setValue the value WILL BE ERASED AS THE CACHE WILL TAKE CONTROL OF IT!
		 * @param obj
		 */
		template <class T>
		void setValue(T& obj) {
			if constexpr (is_shared_ptr<std::decay_t<T>>::value) {
				// Get the hash code of the type that the shared_ptr points to
				typeHashCode = typeid(typename T::element_type).hash_code();
				// If T is a std::shared_ptr
				setValueInner(obj);
			} else {
				typeHashCode = typeid(obj).hash_code();
				// If T is not a std::shared_ptr, create one and use it
				auto sharedPtr = std::make_shared<std::decay_t<T>>(std::forward<T>(obj));
				setValueInner(sharedPtr);
			}
		}

	      private:
		template <class T>
		void setValueInner(std::shared_ptr<T>& value_) {
			if constexpr (std::is_base_of<Cachable, T>::value) {
				// Create a shared_ptr<Cachable> from the input value
				std::shared_ptr<Cachable> cachableValue = std::static_pointer_cast<Cachable>(value_);
				value                                   = cachableValue;
			} else {
				value = value_;
			}
		}
		std::any value;
	};

	/**
	 * We hide the implementation as multi index will kill compile time
	 */
	std::any* fetchInner(const std::string& key);

	template <class T>
	void storeInner(const std::string& _key, const T& _value, bool overwrite_ = false, u64 ttl = 60, bool persistent = false) {

		qint64 expireAt = 0;
		if (ttl) {
			expireAt = QDateTime::currentSecsSinceEpoch() + (qint64)ttl;
		}
		Row row;
		row.key = _key;
		row.setValue(_value);
		row.expireAt   = expireAt;
		row.persistent = persistent;
		storeInner(row, overwrite_);
	}

	void storeInner(const APCU::Row& row_, bool overwrite_);
	void remove(const std::string& key);

	template <class T>
	std::shared_ptr<T> fetch(const std::string& key) {
		(void)key;
		auto res = fetchInner(key);
		if (res->has_value()) {
			// std::shared_ptr<T> x;
			// auto&              tp    = typeid(x);
			// auto               hash0 = tp.hash_code();
			// auto               name0 = tp.name();

			// auto& type = res.type();
			// auto  name = type.name();
			// auto  hash = type.hash_code();

			if constexpr (std::is_base_of<Cachable, T>::value) {
				// Cast res to std::shared_ptr<Cachable> and then to std::shared_ptr<T>

				//option 1, this is the first access, we still have a qbytearray stored
				auto el = any_cast<QByteArray>(res);
				if (el) {
					auto payloadPtr = std::make_shared<T>();
					payloadPtr->deserialize(*el);

					// Store the deserialized value back to the cache
					//downconvert to Cachable first
					auto     cachable = std::static_pointer_cast<Cachable>(payloadPtr);
					std::any a1       = 1;
					*res              = cachable;

					return payloadPtr;
				}

				//option 2, already mutated, just dyn cast and return
				auto cachablePtr = any_cast<std::shared_ptr<Cachable>>(*res);
				return std::dynamic_pointer_cast<T>(cachablePtr);
			} else {
				// T is not derived from Cachable, directly cast res to std::shared_ptr<T>
				return any_cast<std::shared_ptr<T>>(res);
			}

			//	return any_cast<std::shared_ptr<T>>(res);
		}
		return nullptr;
	}

	[[nodiscard]] std::string info() const;

	void clear();

	//This is normally used to get all the sessions
	template <class T>
	std::vector<std::shared_ptr<T>> getAllByType() {
		auto                            hashCode = typeid(T).hash_code();
		auto                            vec      = getAllByTypeInner(hashCode);
		std::vector<std::shared_ptr<T>> final;

		for (size_t i = 0; i < vec.size(); i++) {
			auto& el          = vec[i];
			auto  cachablePtr = any_cast<std::shared_ptr<Cachable>>(*el);
			auto  dyn         = std::dynamic_pointer_cast<T>(cachablePtr);
			final.push_back(std::move(dyn));
		}

		return final;
	}

	struct DiskValue {
		quint32    expireAt     = 1;
		size_t     typeHashCode = 0;
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

	std::vector<std::any*> getAllByTypeInner(size_t hashCode);

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

void           storePOD(const std::string& key, const QByteArray& value, uint ttl = 60, bool persistent = false);
FetchPodResult fetchPOD(const QString& key);
FetchPodResult fetchPOD(const std::string& key);

template <typename T>
concept DerivedFromCachable = std::derived_from<T, Cachable>;

template <DerivedFromCachable T>
void apcuStore(const StringAdt& key, const T& obj, uint ttl = 60, bool persistent = false) {
	APCU::getInstance()->storeInner(key, obj, true, ttl, persistent);
}

template <DerivedFromCachable T>
std::shared_ptr<T> apcuFetch(const StringAdt& key) {
	auto a   = APCU::getInstance();
	auto res = a->fetch<T>(key);
	return res;
}

void apcuRemove(const StringAdt& key);

void apcuClear();
int  apcuTest();
