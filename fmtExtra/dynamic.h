#ifndef DYNAMIC_H
#define DYNAMIC_H

#include "fmt/format.h"
#include <QString>

/**
 * @brief it is appaling to me why is not possible stock to have format on a dynamic string
 * @param fmt the string to format
 * @param args the param
 * @return the formatted string
 */
template <typename... T>
[[nodiscard]] std::string format(const std::string_view& fmt, T&&... args) {
	return fmt::vformat(fmt, fmt::make_format_args(args...));
}

template <typename... T>
[[nodiscard]] std::string F(const std::string_view& fmt, T&&... args) {
	return fmt::vformat(fmt, fmt::make_format_args(args...));
}

template <typename... T>
[[nodiscard]] QString F16(const std::string_view& fmt, T&&... args) {
	return QString::fromStdString(fmt::vformat(fmt, fmt::make_format_args(args...)));
}

#endif // DYNAMIC_H
