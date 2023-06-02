#ifndef NOTFOUNDMIXIN_H
#define NOTFOUNDMIXIN_H

#include "fmt/core.h"

template <typename K>
class NotFoundMixin {
	  public:
	using Funtor    = void (*)(const K& key);
	NotFoundMixin() = default;
	explicit NotFoundMixin(Funtor f)
		: notFoundCallback(f){};

	mutable Funtor notFoundCallback = nullptr;

	[[noreturn]] void callNotFoundCallback(const K& key, const std::string location) const {
		if (notFoundCallback) {
			(*notFoundCallback)(key);
		}
		throw ExceptionV2(fmt::format("key >>>{}<<< not found in {}", key, location));
	}
};

#endif // NOTFOUNDMIXIN_H
