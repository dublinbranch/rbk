#pragma once

#include <string>
#include <string_view>

/**
 * Filename for Content-Disposition (and similar headers).
 * Keep [A-Za-z0-9._-], map everything else to '_'. Not HTML-escape.
 */
inline std::string safeFilename(std::string_view raw, std::string_view fallback = "export.csv") {
	std::string out;
	out.reserve(raw.size() < 200 ? raw.size() : 200);
	for (unsigned char c : raw) {
		if (out.size() >= 200) {
			break;
		}
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
		    c == '-') {
			out += static_cast<char>(c);
		} else {
			out += '_';
		}
	}
	if (out.empty() || out == "." || out == "..") {
		return std::string(fallback);
	}
	return out;
}
