#include "QElapsedTimerV2.h"
#include "fmt/format.h"

std::string QElapsedTimerV2::format() {
	auto   elapsed_time    = this->nsecsElapsed();
	double elapsed_seconds = static_cast<double>(elapsed_time) / 1e9;

	// Split the number into integer and fractional parts
	long long int_part        = static_cast<long long>(elapsed_seconds);
	double    fractional_part = (double)elapsed_seconds - (double)int_part;

	// Format integer part with thousand separators
	std::string int_part_str = fmt::format("{}", int_part);
	for (size_t i = int_part_str.length(); i > 3; i -= 3) {
		int_part_str.insert(i - 3, 1, '.');
	}

	// Get the fractional part as a string with precision
	std::string fractional_part_str = fmt::format("{:.9f}", fractional_part).substr(2); // Skip "0."

	fractional_part_str.insert(3, "m");

	fractional_part_str.insert(6 + 2, "µ"); // Adjusted index to account for inserted characters

	fractional_part_str.insert(9 + 3, "n"); // Adjusted index to account for inserted characters

	// Combine the integer and fractional parts
	return int_part_str + '.' + fractional_part_str;
}
