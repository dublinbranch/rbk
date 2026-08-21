#pragma once

#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <string>

/** Serialize JSON for embedding in HTML `<script>` (e.g. `{{{TABLE_DATA}}}`).
 *  Boost.JSON leaves `<` intact; the HTML parser would then close the script at `</script>`. */
inline std::string jsonForHtmlScript(const boost::json::value& jv) {
	const auto raw = boost::json::serialize(jv);
	std::string out;
	out.reserve(raw.size());
	for (char c : raw) {
		switch (c) {
		case '<':
			out += "\\u003c";
			break;
		case '>':
			out += "\\u003e";
			break;
		case '&':
			out += "\\u0026";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}
