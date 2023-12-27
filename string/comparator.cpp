#include "comparator.h"

bool StringCompare::operator()(const std::string& lhs, const std::string& rhs) const {
	return lhs < rhs;
}

bool StringCompare::operator()(const std::string& lhs, std::string_view rhs) const {
	return lhs < rhs;
}

bool StringCompare::operator()(std::string_view lhs, const std::string& rhs) const {
	return lhs < rhs;
}
