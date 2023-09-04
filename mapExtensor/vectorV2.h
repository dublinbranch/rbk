#pragma once

#include <algorithm>
#include <vector>

template <typename T>
class vectorV2 : public std::vector<T> {
      public:
	using vector_parent = std::vector<T>;

	vectorV2() = default;

	vectorV2(std::initializer_list<T> init)
	    : vector_parent(init) {
	}

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

	const T* elementContainedInto(const T& pattern) const {
		for (auto& el : *this) {
			if (pattern.contains(el)) {
				return &el;
			}
		}
		return nullptr;
	}
};
