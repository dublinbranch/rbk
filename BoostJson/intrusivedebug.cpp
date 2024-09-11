#include "intrusivedebug.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

std::string PushMe::compose() {
	return fmt::format("/{}", fmt::join(path, "/"));
}

PushMe::PushMe() {
	PushMe::path.reserve(32);
}

void PushMe::push(const char *str) {
	PushMe::path.push_back(str);
}

void PushMe::pop() {
	PushMe::path.pop_back();
}
