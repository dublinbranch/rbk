#include "util.h"
#include "rbk/string/stringoso.h"
#include <QByteArray>
#include <QDataStream>
#include <QUrl>
#include <boost/algorithm/string/replace.hpp>
#include <charconv>
#include <string>

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

std::string trim(const std::string& str) {
	auto start = str.begin();
	while (start != str.end() && std::isspace(*start)) {
		start++;
	}

	auto end = str.end();
	do {
		end--;
	} while (std::distance(start, end) > 0 && std::isspace(*end));

	return std::string(start, end + 1);
}

std::string toLower(string& request) {
	std::transform(request.begin(), request.end(), request.begin(), ::tolower);
	return request;
}

string toLower(const std::string& request) {
	string copy = request;
	return toLower(copy);
}

string toLower(std::string_view request) {
	return toLower(string(request));
}

std::vector<string> split(const std::string& str, const std::string& delimiter) {
	std::vector<std::string> tokens;
	std::size_t              start = 0;
	std::size_t              end   = str.find(delimiter);

	while (end != std::string::npos) {
		tokens.push_back(str.substr(start, end - start));
		start = end + 1;
		end   = str.find(delimiter, start);
	}

	tokens.push_back(str.substr(start)); // Add the last token
	return tokens;
}

std::string percentEncoding(QByteAdt adt) {
	return QUrl::toPercentEncoding(adt).toStdString();
}
