#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_COMPARATOR_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_COMPARATOR_H

#include <string>

struct StringCompare {
	using is_transparent = void; // Enable heterogeneous lookup

	bool operator()(const std::string& lhs, const std::string& rhs) const;

	bool operator()(const std::string& lhs, std::string_view rhs) const;

	bool operator()(std::string_view lhs, const std::string& rhs) const;
};
#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_COMPARATOR_H
