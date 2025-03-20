#ifndef CONVERTER_H
#define CONVERTER_H

#include "rbk/QStacker/exceptionv2.h"
#include "rbk/concept/concepts.h"
#include <charconv>
#include <fmt/format.h>
#include <system_error>

// Helper function to convert string to any arithmetic type
template <arithmetic T>
T string_to_number(std::string_view str) {
	T result{};

	if constexpr (std::is_same_v<T, bool>) {
		// Handle boolean conversion
		if (str == "true" || str == "1")
			return true;
		if (str == "false" || str == "0")
			return false;
		throw ExceptionV2(fmt::format("Cannot convert '{}' to bool", str));
	} else {
		// Convert string to numeric type T
		try {
			if constexpr (std::is_integral<T>::value) {
				// For integer types
				if constexpr (std::is_unsigned<T>::value) {
					result = static_cast<T>(std::stoull(std::string(str)));
				} else {
					result = static_cast<T>(std::stoll(std::string(str)));
				}
			} else if constexpr (std::is_floating_point<T>::value) {
				// For floating point types
				result = static_cast<T>(std::stod(std::string(str)));
			} else {
				throw ExceptionV2(fmt::format("Cannot convert string to the requested numeric type"));
			}
		} catch (const std::exception& e) {
			throw ExceptionV2(fmt::format("Failed to convert string '{}' to number: {}", str, e.what()));
		}
	}

	return result;
}

#endif // CONVERTER_H
