#include "founded.h"

template <typename V>
Founded<V>::operator bool() const {
	return found;
}
