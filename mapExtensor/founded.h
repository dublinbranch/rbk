#ifndef FOUNDED_H
#define FOUNDED_H

template <typename V>
struct Founded {
	const V* val   = nullptr;
	bool     found = false;

	explicit operator bool() const;
};

#endif // FOUNDED_H
