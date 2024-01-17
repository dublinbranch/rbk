#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_DYNAMIC_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_DYNAMIC_H

#include "fmt/format.h"
#include "rbk/misc/echo.h"
#include <QString>

/**
 * @brief it is appaling to me why is not possible stock to have format on a dynamic string
 * @param fmt the string to format
 * @param args the param
 * @return the formatted string
 */

template <typename... T>
[[nodiscard]] std::string F(const std::string_view& fmt, T&&... args) {
	return fmt::vformat(fmt, fmt::make_format_args(args...));
}

template <typename... T>
void echo(const std::string_view& fmt, T&&... args) {
	auto msg = fmt::vformat(fmt, fmt::make_format_args(args...));
	echo(msg);
}

template <typename... T>
void warn(const std::string_view& fmt, T&&... args) {
	auto msg = fmt::vformat(fmt, fmt::make_format_args(args...));
	warn(msg);
}

template <typename... T>
[[nodiscard]] QString F16(const std::string_view& fmt, T&&... args) {
	return QString::fromStdString(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... T>
[[nodiscard]] QString F16(const QString& fmt, T&&... args) {
	return QString::fromStdString(fmt::vformat(fmt.toStdString(), fmt::make_format_args(args...)));
}

template <typename... T>
[[nodiscard]] QString F16(const char* fmt, T&&... args) {
	return QString::fromStdString(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

template <typename... T>
[[nodiscard]] QByteArray F8(const std::string_view& fmt, T&&... args) {
	return QByteArray::fromStdString(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_FMTEXTRA_DYNAMIC_H
