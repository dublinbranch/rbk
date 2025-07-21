#include "math.h"
#include "extra.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include <boost/json.hpp>
#include <fmt/format.h>

void mergeJson(bj::object& target, const bj::object& mixMe, bool overwrite) {
	for (auto const& item : mixMe) {
		auto const& key   = item.key();
		auto const& value = item.value();

		if (target.contains(key)) {
			if (value.is_object() && target.at(key).is_object()) {
				// Recursively merge sub-objects
				mergeJson(target.at(key).as_object(), value.as_object(), overwrite);
			} else if (value.is_array() && target.at(key).is_array()) {
				// If both are arrays, you can decide to concatenate them or not
				auto&       target_array = target.at(key).as_array();
				auto const& source_array = value.as_array();
				target_array.insert(target_array.end(), source_array.begin(), source_array.end());
			} else if (overwrite) {
				// Overwrite the value if the key exists and overwrite is true
				target.insert_or_assign(key, value);
			}
		} else {
			// Insert the new key-value pair if the key does not exist in the target
			target.insert_or_assign(key, value);
		}
	}
}

boost::json::object subtractJson(const boost::json::object& first, const boost::json::object& second, std::string path) {
	boost::json::object result;

	// Iterate through the first object
	for (const auto& item : first) {
		const auto& key   = item.key();
		const auto& value = item.value();

		// Check if the key exists in the second object
		if (second.contains(key)) {
			const auto& second_value = second.at(key);

			// If both values are objects, recursively subtract
			if (value.is_object() && second_value.is_object()) {
				std::string newPath = fmt::format("{}/{}", path, std::string_view(key));
				result[key]         = subtractJson(value.as_object(), second_value.as_object(), newPath);
			}
			// If both values are arrays, subtract elements
			else if (value.is_array() && second_value.is_array()) {
				boost::json::array diff_array;
				const auto&        first_array  = value.as_array();
				const auto&        second_array = second_value.as_array();

				for (const auto& elem : first_array) {
					if (std::find(second_array.begin(), second_array.end(), elem) == second_array.end()) {
						diff_array.push_back(elem);
					}
				}
				result[key] = diff_array;
			}
			// For other types, remove if TYPE matches
			else if (value.kind() == second_value.kind()) {

			} else {
				// For integer types, check if they are convertible, because if the value is 0 in the config, is considered an int64, even if the original type was uint64

				if (second_value.kind() == boost::json::kind::uint64) {
					std::error_code ec;
					value.to_number<uint64_t>(ec);
					if (!ec) {
						continue;
					}
				}
				throw std::runtime_error(
				    fmt::format("Types do not match for key: {} in path {}\nFound {} expected {}\nValues are: {} and: {}",
				                std::string_view(key), path, asSWString(value.kind()), asSWString(second_value.kind()),
				                pretty_print(value), pretty_print(second_value)));
			}
		} else {
			// If the key is not in the second object, keep it in the result
			result[key] = value;
		}
	}

	return result;
}
