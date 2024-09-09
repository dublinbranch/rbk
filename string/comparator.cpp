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

bool StringEqual::operator()(std::string_view lhs, std::string_view rhs) const noexcept {
	return lhs == rhs;
}

bool StringEqual::operator()(const std::string& lhs, std::string_view rhs) const noexcept {
	return std::string_view(lhs) == rhs;
}

bool StringEqual::operator()(std::string_view lhs, const std::string& rhs) const noexcept {
	return lhs == std::string_view(rhs);
}

bool StringEqual::operator()(const std::string& lhs, const std::string& rhs) const noexcept {
	return lhs == rhs;
}
