#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_CUSTOMFORMATTER_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_CUSTOMFORMATTER_H

//Looks like is not possible to have those function in a .cpp file for the moment

#include "fmt/format.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/string/stringoso.h"
#include <QDate>
#include <QString>
#include <filesystem>

template <>
struct fmt::formatter<QStringView> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const QStringView& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.toString().toStdString(), ctx);
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
struct fmt::formatter<QStringAdt> : formatter<string_view> {
	template <typename FormatContext>
        auto format(const QStringAdt& p, FormatContext& ctx) const {
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

template <>
struct fmt::formatter<QByteAdt> : formatter<string_view> {
	template <typename FormatContext>
        auto format(const QByteAdt& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.toStdString(), ctx);
	}
};

template <>
struct fmt::formatter<std::filesystem::__cxx11::path> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const std::filesystem::__cxx11::path& p, FormatContext& ctx) const {
		return formatter<string_view>::format(p.string(), ctx);
	}
};

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_CUSTOMFORMATTER_H
