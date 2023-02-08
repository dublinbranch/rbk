#pragma once
#include "rbk/defines/stringDefine.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/mapExtensor/qmapV2.h"
#include "rbk/misc/swapType.h"

//TODO move to mapV2 as most of the stuff here is now duplicated

class sqlRow : public QMapV2<QByteArray, QByteArray> {
      public:
	bool fromCache = false;

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

	QByteArray rq(const QByteArray& key) const {
		QByteArray temp;
		get(key, temp);
		return temp;
	}

	template <typename T>
	[[nodiscard]] T rq(const QByteArray& key) const {
		QByteArray temp;
		get(key, temp);
		T t2;
		swapType(temp, t2);
		return t2;
	}

	/**
	 * @brief rqe is specific for int type enum
	 * @param key
	 * @return
	 */
	template <isEnum T>
	[[nodiscard]] T rqe(const QByteArray& key) const {
		QByteArray temp;
		get(key, temp);
		T t2 = T(temp.toInt());
		return t2;
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
	D get2(const QByteArray& key) const {
		QByteArray temp;
		D          temp2;
		get(key, temp);
		swapType(temp, temp2);
		return temp2;
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
