#pragma once

#include <vector>

template <typename T>
class vectorV2 : public std::vector<T> {
      public:
	template <typename N>
	void append(const N& n) {
		this->insert(this->end(), n.begin(), n.end());
	}
};
