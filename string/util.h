#pragma once

#include "rbk/number/intTypes.h"
#include <optional>
#include <string>
#include <vector>

class QByteArray;
class QByteAdt;
class StringAdt;

std::string_view         subView(const std::string& string, size_t start, size_t end);
void                     replace(const std::string& search, const std::string& replace, std::string& string);
std::optional<i64>       stoi(const std::string_view& input);
QByteArray               removeNonAscii(const QByteArray& input);
std::string              trim(const std::string& str);
std::string              toLower(const std::string& request);
std::string              toLower(std::string& request);
std::string              toLower(std::string_view request);
std::vector<std::string> split(const std::string& str, const std::string& delimiter);
std::string              percentEncoding(QByteAdt adt);
