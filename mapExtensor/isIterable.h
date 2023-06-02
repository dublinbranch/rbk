#ifndef ISITERABLE_H
#define ISITERABLE_H


template <class T>
concept isIterable = requires(const T& t) {
	t.begin();
	t.end();
};


#endif // ISITERABLE_H
