#ifndef DECIMATE_H
#define DECIMATE_H

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

#endif // DECIMATE_H
