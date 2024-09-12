#pragma once

#include <string>
#include <string_view>

struct StringHash {
	using is_transparent = void; // Enables heterogeneous lookup

	std::size_t operator()(std::string_view s) const noexcept;

	std::size_t operator()(const std::string& s) const noexcept;

	std::size_t operator()(const char* s) const noexcept;
};

