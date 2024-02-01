#pragma once

#include "fmt/format.h"
#include "fmt/ranges.h"
#include "magic_enum.hpp"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/concept/concepts.h"
#include <QByteArray>
#include <QString>
#include <vector>

//Todo is possible to do magic_enum::enum_name(INVALID_VALUE) which will silently fail and return an empty string, not what we want!

namespace magic_enum {
template <typename Type>
std::vector<Type> getEnabled(const Type& t) {
	static constexpr auto& values = magic_enum::enum_values<Type>();
	std::vector<Type>      set;
	for (const auto& c : values) {
		using namespace bitwise_operators;
		if (to_underlying(t & c)) {
			set.push_back(c);
		}
	}
	return set;
}

template <typename Type>
std::vector<std::string_view> asString(const std::vector<Type>& t) {
	std::vector<std::string_view> res;
	for (const auto& c : t) {
		res.push_back(enum_name(c));
	}
	return res;
}

template <typename Type>
std::string composeError(std::string_view key, Type) {
	auto names = enum_names<Type>();
	return fmt::format("The key >>>{}<<< is not contained in the enum >>>{}<<<", key, fmt::join(names, " - "));
}

template <typename T>
[[nodiscard]] T fromString(const QString& _string) {
	return enum_cast<T>(_string.toStdString()).value();
}

template <typename T>
[[nodiscard]] T fromString(const std::string& _string) {
	return enum_cast<T>(_string).value();
}

template <typename T>
[[nodiscard]] T fromString(const std::string_view& _string) {
	return enum_cast<T>(_string).value();
}

template <typename T>
void fromString(const std::string& _string, T& t) {
	auto opt = enum_cast<T>(_string);
	if (opt.has_value()) {
		t = opt.value();
	} else {
		auto msg = composeError(_string, t);
		throw ExceptionV2(msg);
	}
}

template <typename T>
void fromString(const QString& _string, T& t) {
	fromString(_string.toStdString(), t);
}

template <typename E>
[[nodiscard]] QString enum_nameQS(E e) {
	std::string copy(enum_name(e));
	return QString::fromStdString(copy);
}

} // namespace magic_enum

template <isEnum T>
[[nodiscard]] QString asString(T t) {
	return magic_enum::enum_nameQS(t);
}

template <isEnum T>
[[nodiscard]] std::string_view asSWString(T t) {
	return magic_enum::enum_name(t);
}
