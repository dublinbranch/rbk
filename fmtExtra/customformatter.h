#ifndef CUSTOMFORMATTER_H
#define CUSTOMFORMATTER_H

//Looks like is not possible to have those function in a .cpp file for the moment

#include "fmt/format.h"
#include "rbk/defines/stringDefine.h"
#include <QDate>
#include <QString>

template <>
struct fmt::formatter<QStringRef> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStringRef& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.string()->toStdString(), ctx);
	}
};

template <>
struct fmt::formatter<QDate> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QDate& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.toString(mysqlDateFormat).toStdString(), ctx);
	}
};

template <>
struct fmt::formatter<QString> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QString& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.toStdString(), ctx);
	}
};

template <>
struct fmt::formatter<QByteArray> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QByteArray& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.toStdString(), ctx);
	}
};

#endif // CUSTOMFORMATTER_H
