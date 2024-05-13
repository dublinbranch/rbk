#include "util.h"
#include <QByteArray>
#include <boost/algorithm/string/replace.hpp>
#include <charconv>

using namespace std;

std::string_view subView(const std::string& string, size_t start, size_t end) {
	std::string_view res(string);
	return res.substr(start, end - start);
}

void replace(const std::string& search, const std::string& replace, std::string& string) {
	boost::algorithm::replace_all(string, search, replace);
}

QByteArray removeNonAscii(const QByteArray& input) {
	QByteArray result;
	result.reserve(input.size()); // Optional but improves efficiency
	for (unsigned char c : input) {
		if (c < 128) { // Check if byte is an ASCII character
			result.append(c);
		}
	}
	return result;
}

std::optional<i64> stoi(const std::string_view& input) {
	i64                          out    = 0;
	const std::from_chars_result result = std::from_chars(input.data(), input.data() + input.size(), out);
	if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
		return std::nullopt;
	}
	return out;
}
