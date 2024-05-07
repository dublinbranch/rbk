#pragma once

#include "rbk/number/intTypes.h"
#include <optional>
#include <string>

class QByteArray;

std::string_view   subView(const std::string& string, size_t start, size_t end);
void               replace(const std::string& search, const std::string& replace, std::string& string);
std::optional<i64> stoi(const std::string_view& input);
QByteArray         removeNonAscii(const QByteArray& input);
