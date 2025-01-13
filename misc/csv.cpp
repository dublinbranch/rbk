#include "csv.h"

std::string escapeForCSV(const std::string_view& input) {
	// We'll build a new string that:
	// 1) Doubles any existing double quotes.
	// 2) Tracks if we need to enclose the entire field in double quotes
	//    (if the field has a comma, newline, or quote).
	bool        mustQuote = false;
	std::string output;
	output.reserve(input.size() * 2); // Reserve some space to reduce reallocations

	for (char c : input) {
		switch (c) {
		case '"':
			// Double any existing double quote
			output += "\"\"";
			mustQuote = true;
			break;
		case ',':
		case '\n':
		case '\r':
			// If there's a comma or newline, we need to enclose the whole field in quotes
			output += c;
			mustQuote = true;
			break;
		default:
			output += c;
			break;
		}
	}

	// If we found any character that requires quoting, or if the string is empty,
	// enclose the entire field in double quotes.
	if (mustQuote || output.empty()) {
		return "\"" + output + "\"";
	} else {
		return output;
	}
}
