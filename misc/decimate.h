#ifndef DECIMATE_H
#define DECIMATE_H

#include <vector>

template <typename Container>
Container decimate(const Container& input, i64 max_count) {
	Container result;
	i64       skip = 0;
	i64       i    = 0;

	// Check if decimation is needed
	if (input.size() > max_count) {
		skip = input.size() / max_count;
	}

	// Apply decimation if needed
	for (const auto& item : input) {
		if (skip == 0 || (i++ % skip == 0)) {
			result.push_back(item);
		}
	}

	return result;
}

/** Same stride as decimate(), for column-store rows. */
inline std::vector<size_t> decimateIndices(size_t n, i64 max_count) {
	std::vector<size_t> result;
	i64                 skip = 0;
	i64                 i    = 0;
	if (max_count > 0 && n > static_cast<size_t>(max_count)) {
		skip = static_cast<i64>(n) / max_count;
	}
	result.reserve(max_count > 0 && n > static_cast<size_t>(max_count) ? static_cast<size_t>(max_count) + 1 : n);
	for (size_t r = 0; r < n; ++r) {
		if (skip == 0 || (i++ % skip == 0)) {
			result.push_back(r);
		}
	}
	return result;
}

#endif // DECIMATE_H
