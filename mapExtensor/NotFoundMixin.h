#ifndef NOTFOUNDMIXIN_H
#define NOTFOUNDMIXIN_H

#include "rbk/QStacker/exceptionv2.h"
#include <fmt/core.h>
#include <functional>

template <typename K>
class NotFoundMixin {
	  public:
	using Funtor    = std::function<void(const K&)>;
	NotFoundMixin() = default;
	explicit NotFoundMixin(Funtor f)
		: notFoundCallback(f){};

	mutable Funtor notFoundCallback = nullptr;

	[[noreturn]] void callNotFoundCallback(const K& key, const std::string location) const {
		if (notFoundCallback) {
			notFoundCallback(key);
		}
		std::string msg = fmt::format("key >>>{}<<< not found in {}", key, location);
		throw ExceptionV2(msg);
	}
};

#endif // NOTFOUNDMIXIN_H
