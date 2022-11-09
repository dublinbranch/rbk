#pragma once
#include "rbk/defines/stringDefine.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/mapExtensor/qmapV2.h"

class sqlRow : public QMapV2<QByteArray, QByteArray> {
      public:
	bool fromCache = false;

	template <typename D>
	void rq(const QByteArray& key, D& dest, const D* def = nullptr) const {
		QByteArray temp;
		//If the value is found perform the conversion
		if (getReal(key, temp)) {
			swap(temp, dest);
		}
		//Else just return the default if is set
		if (def) {
			dest = *def;
		}
	}

	template <typename T>
	T rq(const QByteArray& key) const {
		QByteArray temp;
		get(key, temp);
		T t2;
		swap(temp, t2);
		return t2;
	}

	QDateTime asDateTime(const QByteArray& key) const;

	template <typename D>
	[[deprecated("use rq")]] void get2(const QByteArray& key, D& dest) const {
		rq(key, dest);
	}

	// To avoid conversion back and forth QBytearray of the default value and the his result
	template <typename D>
	bool get2(const QByteArray& key, D& dest, const D& def) const {
		if (auto v = this->fetch(key); v) {
			swap(*v.value, dest);
			return true;
		}
		dest = def;
		return false;
	}

	template <typename D>
	bool getIfNotNull(const QByteArray& key, D& dest, const D& def) const {
		auto iter = find(key);
		if (iter == end()) {
			dest = def;
			return false;
		}
		if (iter.value().toUpper() == "NULL") {
			dest = def;
			return false;
		}
		swap(iter.value(), dest);
		return true;
	}

	template <typename D>
	D get2(const QByteArray& key) const {
		QByteArray temp;
		D          temp2;
		get(key, temp);
		swap(temp, temp2);
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

      private:
	template <typename D>
	void swap(const QByteArray& source, D& dest) const {
		if constexpr (std::is_same<D, QString>::value) {
			dest = QString(source);
			return;
		} else if constexpr (std::is_same<D, QByteArray>::value) {
			dest = source;
			return;
		} else if constexpr (std::is_same<D, std::string>::value) {
			dest = source.toStdString();
			return;
		} else if constexpr (std::is_same<D, QDate>::value) {
			dest = QDate::fromString(source, mysqlDateFormat);
			return;
		} else if constexpr (std::is_same<D, QDateTime>::value) {
			dest = QDateTime::fromString(source, mysqlDateTimeFormat);
			return;
		} else if constexpr (std::is_enum_v<D>) {
			auto s = source.toStdString();
			magic_enum::fromString(s, dest);
			return;
		} else if constexpr (std::is_arithmetic_v<D>) {
			bool ok = false;
			if constexpr (std::is_floating_point_v<D>) {
				dest = source.toDouble(&ok);
			} else if constexpr (std::is_signed_v<D>) {
				dest = source.toLongLong(&ok);
			} else if constexpr (std::is_unsigned_v<D>) {
				dest = source.toULongLong(&ok);
			}
			if (!ok) {
				// last chanche NULL is 0 in case we are numeric right ?
				if (source == QBL("NULL")) {
					if constexpr (std::is_arithmetic_v<D>) {
						dest = 0;
						return;
					}
				}
				throw QSL("Impossible to convert %1 as a number").arg(QString(source));
			}
		} else {
			// poor man static assert that will also print for which type it failed
			typedef typename D::something_made_up X;

			X y;     // To avoid complain that X is defined but not used
			(void)y; // TO avoid complain that y is unused
		}
	}
};
