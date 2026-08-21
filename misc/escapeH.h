#pragma once

#include <string>
#include <string_view>

/** HTML-escape for values interpolated into markup (same set as Boost.Mustache `{{ }}`). */
inline std::string escapeH(std::string_view s) {
	std::string out;
	out.reserve(s.size());
	for (char c : s) {
		switch (c) {
		case '&':
			out += "&amp;";
			break;
		case '<':
			out += "&lt;";
			break;
		case '>':
			out += "&gt;";
			break;
		case '"':
			out += "&quot;";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}
