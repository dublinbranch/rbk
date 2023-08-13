#pragma once

#include <algorithm>
#include <vector>

template <typename T>
class vectorV2 : public std::vector<T> {
      public:
	template <typename N>
	void append(const N& n) {
		this->insert(this->end(), n.begin(), n.end());
	}

	template <typename N>
	bool contain(const N& n) {
		return std::find(this->begin(), this->end(), n) != this->end();
	}

	template <typename N>
	bool try_insert(const N& n) {
		if (!contain(n)) {
			this->push_back(n);
			return true;
		}
		return false;
	}
};
