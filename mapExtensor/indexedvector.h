#ifndef INDEXEDVECTOR_H
#define INDEXEDVECTOR_H
#include "rbk/QStacker/exceptionv2.h"
#include <cassert>
#include <map>
#include <stdint.h>
#include "isIterable.h"
//This is to keep the same interface of a vector, but behave internally more like a map for faster access

template <class T>
concept isIndexedVector = requires(const T& t) {
	t.isIndexedVector;
};

template <class T>
class indexedVector {
	using innerMap = std::multimap<int64_t, T>;

      private:
	innerMap content;

      public:
	const innerMap& get() const {
		return content;
	}
	innerMap& get() {
		return content;
	}
	void push_back(const T& r) {
		constexpr bool hasRty = requires() {
			r.rty;
		};
		constexpr bool hasRty_ptr = requires() {
			r->rty;
		};
		constexpr bool hasHeader = requires() {
			r.header.rty;
		};
		constexpr bool hasHeader_ptr = requires() {
			r->header.rty;
		};
		constexpr bool hasCampaign = requires() {
			r->campaign->rty;
		};
		constexpr bool hasHeaderCampaign = requires() {
			r->header.campaign->rty;
		};

		//Due to an orrible error that Roy did we now need this hack, well not entirely
		if constexpr (hasRty) {
			content.insert({r.rty, r});
			return;
		} else if constexpr (hasRty_ptr) {
			content.insert({r->rty, r});
			return;
		} else if constexpr (hasHeader) {
			content.insert({r.header.rty, r});
			return;
		} else if constexpr (hasHeader_ptr) {
			content.insert({r->header.rty, r});
			return;
		} else if constexpr (hasCampaign) {
			content.insert({r->campaign->rty, r});
			return;
		} else if constexpr (hasHeaderCampaign) {
			content.insert({r->header.campaign->rty, r});
			return;
		}
		throw ExceptionV2("what is that now ?");
	}

	void push_back(isIterable auto& n) {
		for (auto& x : n) {
			this->push_back(x);
		}
	}

	struct Iterator {
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = size_t;
		using value_type        = T;
		using pointer           = typename innerMap::iterator; // or also value_type*
		using reference         = T&;                          // or also value_type&

		Iterator() = default;

		static Iterator dummy() {
			return Iterator();
		}
		[[nodiscard]] Iterator(pointer ptr)
		    : m_ptr(ptr) {
		}

		[[nodiscard]] int64_t key() const {
			return m_ptr->first;
		}
		reference operator*() const {
			return m_ptr->second;
		}
		//we hide the indexing part, also this is very nice to be used for shared ptr a single -> dereferences both!
		T* operator->() const {
			return &m_ptr->second;
		}

		// Prefix increment
		Iterator& operator++() {
			m_ptr++;
			return *this;
		}

		Iterator after() {
			auto tmp = *this;
			tmp++;
			return tmp;
		}

		// Postfix increment
		Iterator operator++(int) {
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		friend bool operator==(const Iterator& a, const Iterator& b) {
			return a.m_ptr == b.m_ptr;
		};

		friend bool operator!=(const Iterator& a, const Iterator& b) {
			return a.m_ptr != b.m_ptr;
		};

	      private:
		pointer m_ptr;
	};

	//poor man tag dispatch for concepts ?
	static bool isIndexedVector;

	[[nodiscard]] bool empty() const {
		return content.empty();
	}

	void clear() {
		content.clear();
	}

	[[nodiscard]] size_t size() const {
		return content.size();
	}

	//this is just a convenience function to access the inner shared ptr from an iterator
	//of course this need to be if constexpressed
	const auto& firstN() const {
		return content.begin()->second;
	}
	const auto& operator[](size_t t) const {
		return at(t);
	}
	const auto& at(size_t t) const {
		if (t != 0) {
			assert("not supported! this is just a convenience function");
		}
		return content.begin()->second;
	}
	Iterator begin() {
		return Iterator(content.begin());
	}
	Iterator end() {
		return Iterator(content.end());
	}
	const Iterator begin() const {
		return cbegin();
	}
	const Iterator end() const {
		return cend();
	}

	const Iterator cbegin() const {
		//not sure what is going on here
		auto c = content.cbegin();
		return Iterator(c._M_const_cast());
	}
	const Iterator cend() const {
		//not sure what is going on here
		return Iterator(content.cend()._M_const_cast());
	}
};

#endif // INDEXEDVECTOR_H
