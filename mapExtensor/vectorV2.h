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
	bool contains(const N& n) const {
		return std::find(this->begin(), this->end(), n) != this->end();
	}

	template <typename N>
	bool try_insert(const N& n) {
		if (!contains(n)) {
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

	/**
	 * @brief Find the first element that contains the given element
	 * @param n The element to find
	 * @return A pointer to the element if found, nullptr otherwise
	 */
	template <typename N>
	T* findN(const N& n) {
		auto it = std::find(this->begin(), this->end(), n);
		if (it != this->end()) {
			return &*it;
		}
		return nullptr;
	}

	/**
	 * @brief Find the first element that contains the given element
	 * @param n The element to find
	 * @return A pointer to the element if found, nullptr otherwise
	 */
	template <typename N>
	const T* findN(const N& n) const {
		auto it = std::find(this->begin(), this->end(), n);
		if (it != this->end()) {
			return &*it;
		}
		return nullptr;
	}
};
