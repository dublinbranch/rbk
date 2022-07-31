#ifndef FIXEDSIZEVECTOR_H
#define FIXEDSIZEVECTOR_H
#include "vectorV2.h"

template <typename T>
class FixedVector : public vectorV2<T> {
      public:
	FixedVector(size_t ms) {
		maxSize = ms;
	}
	void   append()       = delete;
	void   push_back()    = delete;
	void   emplace_back() = delete;
	size_t tryAppend(const T& value) {
		auto s = vectorV2<T>::size();
		if (s >= maxSize) {
			return false;
		}
		vectorV2<T>::push_back(value);
		return s++;
	}
	bool full() const {
		return vectorV2<T>::size() >= maxSize;
	}

      private:
	size_t maxSize = 0;
};

#endif // FIXEDSIZEVECTOR_H
