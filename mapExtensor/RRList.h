#ifndef LISTV2_H
#define LISTV2_H

#include <list>

template <typename T>
class RRList : public std::list<T> {
      public:
	RRList(size_t _maxSize) {
		maxSize = _maxSize;
	}
	size_t maxSize = 0;

	void append(const T& value) {
		this->push_back(value);
		if (maxSize && maxSize < this->size()) {
			this->pop_front();
		}
	}
};
#endif // LISTV2_H
