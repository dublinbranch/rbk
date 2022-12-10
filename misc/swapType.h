#ifndef SWAPTYPE_H
#define SWAPTYPE_H
#include "rbk/defines/stringDefine.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QString>

template <typename D>
void swapType(const QByteArray& source, D& dest) {
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
				dest = 0;
				return;
			}
			//I think an empty string can be safely considered 0 in all cases
			if (source.isEmpty()) {
				dest = 0;
				return;
			}
			throw ExceptionV2(QSL("Impossible to convert >>>%1<<< as a number").arg(QString(source)));
		}
	} else {
		// poor man static assert that will also print for which type it failed
		using X = typename D::something_made_up;

		X y;     // To avoid complain that X is defined but not used
		(void)y; // TO avoid complain that y is unused
	}
}

template <typename D>
void swapType(const QString& source, D& dest) {
	if constexpr (std::is_same<D, QString>::value) {
		dest = source;
		return;
	} else if constexpr (std::is_same<D, QByteArray>::value) {
		dest = source.toUtf8();
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
			//I think an empty string can be safely considered 0 in all cases
			if (source.isEmpty()) {
				dest = 0;
				return;
			}
			throw ExceptionV2(QSL("Impossible to convert >>>%1<<< as a number").arg(QString(source)));
		}
	} else {
		// poor man static assert that will also print for which type it failed
		using X = typename D::something_made_up;

		X y;     // To avoid complain that X is defined but not used
		(void)y; // TO avoid complain that y is unused
	}
}

template <typename V, typename K>
V swapType(const K& k) {
	V v;
	swapType(k, v);
	return v;
}

#endif // SWAPTYPE_H
