#pragma once

#include <string>

std::string_view subView(const std::string& string, size_t start, size_t end);
void             replace(const std::string& search, const std::string& replace, std::string& string);
