#ifndef SWAPTYPE_H
#define SWAPTYPE_H
#include "rbk/concept/concepts.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/magicEnum/BetterEnum.hpp"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/misc/typeinfo.h"
#include "rbk/types/isOptional.h"
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
		bool convertible;
		auto val = source.toUInt(&convertible);
		if (val && convertible) {
			dest.setSecsSinceEpoch(val);
		} else {
			dest = QDateTime::fromString(source, mysqlDateTimeFormat);
		}
		return;
	} else if constexpr (std::is_enum_v<D>) {
		auto s = source.toStdString();
		//NULL can not be used as an ENUM value, we use in that case NA
		if (s == "NULL") {
			dest = D::NA;
		} else {
			magic_enum::fromString(s, dest);
		}

		return;
		//} else if constexpr (isBetterEnum<D>) {
	} else if constexpr (std::is_arithmetic_v<D>) {
		bool ok = false;
		if constexpr (std::is_floating_point_v<D>) {
			dest = source.toDouble(&ok);
		} else if constexpr (std::is_signed_v<D>) {

#pragma GCC diagnostic push
//we should check if is not overflowing tbh
#pragma GCC diagnostic ignored "-Wconversion"
			dest = source.toLongLong(&ok);
#pragma GCC diagnostic pop

		} else if constexpr (std::is_unsigned_v<D>) {

#pragma GCC diagnostic push
//we should check if is not overflowing tbh
#pragma GCC diagnostic ignored "-Wconversion"
			dest = source.toULongLong(&ok);
#pragma GCC diagnostic pop
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
			//dest
			throw ExceptionV2(QSL("Impossible to convert >>>%1<<< as a %2").arg(QString(source)).arg(QString::fromStdString(getTypeName<D>())));
		}
	} else if constexpr (is_optional<D>) {
		typename D::value_type t;
		if (source == BSQL_NULL) {
			dest.reset();
			return;
		}
		swapType(source, t);
		dest = t;
	} else if constexpr (TagInvokable<D>) {
		RBK::swapTypeToTag<D> tag;
		dest = tag_invoke(tag, source);
	} else {
		// poor man static assert that will also print for which type it failed
		using X = typename D::something_made_up;

		X y;     // To avoid complain that X is defined but not used
		(void)y; // TO avoid complain that y is unused
	}
}

template <typename D>
void swapType(const std::string& source, D& dest) {
	if constexpr (std::is_same<D, QString>::value) {
		dest = QString::fromStdString(source);
		return;
	} else if constexpr (std::is_same<D, QByteArray>::value) {
		dest = source;
		return;
	} else if constexpr (std::is_same<D, std::string>::value) {
		dest = source;
		return;
	} else if constexpr (std::is_same<D, QDate>::value) {
		dest = QDate::fromString(QString::fromStdString(source), mysqlDateFormat);
		return;
	} else if constexpr (std::is_same<D, QDateTime>::value) {
		bool convertible;
		auto val = QByteArrayView(source.data(), source.length()).toUInt(&convertible);
		if (val && convertible) {
			dest.setSecsSinceEpoch(val);
		} else {
			dest = QDateTime::fromString(QString::fromStdString(source), mysqlDateTimeFormat);
		}
		return;
	} else if constexpr (std::is_enum_v<D>) {
		auto s = source;
		//NULL can not be used as an ENUM value, we use in that case NA
		if (s == "NULL") {
			dest = D::NA;
		} else {
			magic_enum::fromString(s, dest);
		}

		return;
		//} else if constexpr (isBetterEnum<D>) {
	} else if constexpr (std::is_arithmetic_v<D>) {
		bool        ok = false;
		const auto& qb = QByteArrayView(source.data(), source.length());
		if constexpr (std::is_floating_point_v<D>) {
			dest = qb.toDouble(&ok);
		} else if constexpr (std::is_signed_v<D>) {

#pragma GCC diagnostic push
//we should check if is not overflowing tbh
#pragma GCC diagnostic ignored "-Wconversion"
			dest = qb.toLongLong(&ok);
#pragma GCC diagnostic pop

		} else if constexpr (std::is_unsigned_v<D>) {

#pragma GCC diagnostic push
//we should check if is not overflowing tbh
#pragma GCC diagnostic ignored "-Wconversion"
			dest = qb.toULongLong(&ok);
#pragma GCC diagnostic pop
		}
		if (!ok) {
			// last chanche NULL is 0 in case we are numeric right ?
			if (source == S_SQL_NULL) {
				dest = 0;
				return;
			}
			//I think an empty string can be safely considered 0 in all cases
			if (source.empty()) {
				dest = 0;
				return;
			}
			//dest
			throw ExceptionV2(fmt::format("Impossible to convert >>>{}<<< as a {}", source, getTypeName<D>()));
		}
	} else if constexpr (is_optional<D>) {
		typename D::value_type t;
		if (source == S_SQL_NULL) {
			dest.reset();
			return;
		}
		swapType(source, t);
		dest = t;
	} else if constexpr (TagInvokable<D>) {
		RBK::swapTypeToTag<D> tag;
		dest = tag_invoke(tag, source);
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
	} else if constexpr (std::is_same<D, bool>::value) {
		//bool can be expressed in many different ways...
		if (source == QSL("1") || source == QSL("true") || source == QSL("yes")) {
			dest = true;
		} else if (source == QSL("0") || source == QSL("false") || source == QSL("no")) {
			dest = false;
		} else {
			throw ExceptionV2(QSL("Impossible to convert >>>%1<<< as a boolean").arg(source));
		}
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
#pragma GCC diagnostic push
//many false warning here
#pragma GCC diagnostic ignored "-Wconversion"

			dest = source.toULongLong(&ok);

#pragma GCC diagnostic pop
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
	} else if constexpr (is_optional<D>) {
		typename D::value_type t;
		if (source == SQL_NULL) {
			dest.reset();
			return;
		}
		swapType(source, t);
		dest = t;
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
