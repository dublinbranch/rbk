#include "isjsonasubset.h"
#include "to_string.h"
#include <boost/json.hpp>
#include <fmt/core.h>

void isJsonASubset(const boost::json::value& outer, const boost::json::value& subset, const std::string& basePath) {

	switch (subset.kind()) {
	case boost::json::kind::object: {
		auto const& obj = subset.get_object();
		if (!obj.empty()) {
			auto it = obj.begin();
			for (;;) {
				std::string path = fmt::format("{}/{}", basePath, std::string_view(it->key()));
				isJsonASubset(outer, it->value(), path);
				if (++it == obj.end()) {
					break;
				}
			}
		}

		break;
	}

	case boost::json::kind::array: {

		auto const& arr = subset.get_array();
		if (!arr.empty()) {
			auto it = arr.begin();
			int  i  = 0;
			for (;;) {
				std::string path = fmt::format("{}/{}", basePath, i);
				isJsonASubset(outer, *it, path);
				i++;
				if (++it == arr.end())
					break;
			}
		}

		break;
	}
	default:

		boost::json::error_code ec;
		if (auto inner = outer.find_pointer(basePath, ec); inner) {
			if (*inner != subset) {
				fmt::print("Mismatch in {} we have {} instead of {}\n", basePath, to_string(*inner), to_string(subset));
				return;
			}
		} else {
			fmt::print("This is missing {} \n", basePath);
			return;
		}
	}
}
