#pragma once
#include "rbk/defines/stringDefine.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/mapExtensor/qmapV2.h"
#include "rbk/misc/intTypes.h"
#include "rbk/misc/swapType.h"

//TODO move to mapV2 as most of the stuff here is now duplicated

class sqlRow : public QMapV2<QByteArray, QByteArray> {
      public:
	bool fromCache = false;

	[[nodiscard]] QByteArray rq(const QByteArray& key) const {
		QByteArray v;
		getReal(key, v);
		return v;
	}

	template <typename D>
	void rq(const QByteArray& key, D& dest, const D* def = nullptr) const {
		QByteArray temp;
		//If the value is found perform the conversion
		if (getReal(key, temp)) {
			swapType(temp, dest);
		}
		//Else just return the default if is set
		if (def) {
			dest = *def;
		}
	}

	template <typename T>
	[[nodiscard]] T rq(const QByteArray& key) const {
		T temp;
		rq(key, temp);
		return temp;
	}

	/**
	 * @brief rqe is specific for int type enum IE the one NOT using ENUM but an actual INT inside mysql
	 * @param key
	 * @return
	 */
	template <isEnum T>
	[[nodiscard]] T rqe(const QByteArray& key) const {
		i64 temp = 0;
		rq(key, temp);
		T t2 = T(temp);
		return t2;
	}

	template <isEnum T>
	void rqe(const QByteArray& key, T& t) const {
		i64 temp = 0;
		rq(key, temp);
		t = T(temp);
	}

	QDateTime asDateTime(const QByteArray& key) const;

	template <typename D>
	bool get2(const QByteArray& key, D& dest) const {
		if (auto v = this->fetch(key); v) {
			swapType(*v.value, dest);
			return true;
		}
		return false;
	}

	// To avoid conversion back and forth QBytearray of the default value and the his result
	template <typename D>
	bool get2(const QByteArray& key, D& dest, const D& def) const {
		if (auto v = this->fetch(key); v) {
			swapType(*v.value, dest);
			return true;
		}
		dest = def;
		return false;
	}

	template <typename D>
	bool getIfNotNull(const QByteArray& key, D& dest, const D& def) const {
		auto iter = find(key);
		if (iter == ParentMap::end()) {
			dest = def;
			return false;
		}
		if (iter.value().toUpper() == "NULL") {
			dest = def;
			return false;
		}
		swapType(iter.value(), dest);
		return true;
	}

	template <typename D>
	bool getIfNotNull(const QByteArray& key, D& dest) const {
		return getIfNotNull(key, dest, dest);
	}

	template <typename D>
	D getIfNotNull(const QByteArray& key, const D& def) const {
		D temp;
		getIfNotNull(key, temp, def);
		return temp;
	}

	template <typename D>
	[[nodiscard]] D get2(const QByteArray& key) const {
		QByteArray temp;
		D          temp2;
		get(key, temp);
		swapType(temp, temp2);
		return temp2;
	}

	template <typename D>
	[[nodiscard]] D get2(const QByteArray& key, const D& def) const {
		D val;
		get2(key, val, def);
		return val;
	}

	// Sooo many time we need a QString back
	QString g16(const QByteArray& key) const {
		return get2<QString>(key);
	}

	// Sooo many time we need a QString back
	QString g16(const QByteArray& key, const QString& def) const {
		QString val;
		get2(key, val, def);
		return val;
	}
	// When you can not use operator <<
	QString serialize() const;
};
