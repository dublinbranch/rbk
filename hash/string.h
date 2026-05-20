#pragma once

#include <string>
#include <string_view>

//for the heterogeneous container trick you need those 2
//#include "rbk/hash/string.h"
//#include "rbk/string/comparator.h"

struct StringHash {
	using is_transparent = void; // Enables heterogeneous lookup

	std::size_t operator()(std::string_view s) const noexcept;

	std::size_t operator()(const std::string& s) const noexcept;

	std::size_t operator()(const char* s) const noexcept;
};
