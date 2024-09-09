#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_COMPARATOR_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_COMPARATOR_H

#include <string>

struct StringCompare {
	using is_transparent = void; // Enable heterogeneous lookup

	bool operator()(const std::string& lhs, const std::string& rhs) const;

	bool operator()(const std::string& lhs, std::string_view rhs) const;

	bool operator()(std::string_view lhs, const std::string& rhs) const;
};

struct StringEqual {
	using is_transparent = void; // Enables heterogeneous lookup

	bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;

	bool operator()(const std::string& lhs, std::string_view rhs) const noexcept;

	bool operator()(std::string_view lhs, const std::string& rhs) const noexcept;

	// Overload for both std::string cases to resolve ambiguity
	bool operator()(const std::string& lhs, const std::string& rhs) const noexcept;

	// Template that forwards to std::string_view for any type that can be converted to it
	template <typename T, typename U>
	bool operator()(const T& lhs, const U& rhs) const noexcept {
		return std::string_view(lhs) == std::string_view(rhs);
	}
};

#endif
