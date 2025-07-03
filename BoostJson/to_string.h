#ifndef BOOST__JSON__TO_STRING_H
#define BOOST__JSON__TO_STRING_H

#include "fmt/format.h"
#include <boost/json/string.hpp>

std::string to_string(boost::json::value const* jv);
std::string to_string(boost::json::value const& jv);

template <>
struct fmt::formatter<boost::json::string> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const boost::json::string& p, FormatContext& ctx) const {
		return formatter<string_view>::format(std::string_view(p.data(), p.size()), ctx);
	}
};

#endif // TO_STRING_H
