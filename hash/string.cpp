#include "string.h"
#include "rbk/hash/rapidhash.h"

std::size_t StringHash::operator()(std::string_view s) const noexcept {
	return rapidhash(s.data(), s.size());
}

std::size_t StringHash::operator()(const std::string& s) const noexcept {
	return rapidhash(s.data(), s.size());
}

std::size_t StringHash::operator()(const char* s) const noexcept {
	return rapidhash(s, strlen(s));
}
