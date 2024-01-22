#pragma once

#include "rbk/number/intTypes.h"
#include <QByteArray>
#include <QString>
#include <string>

std::string_view   subView(const std::string& string, size_t start, size_t end);
void               replace(const std::string& search, const std::string& replace, std::string& string);
std::optional<i64> stoi(const std::string_view& input);

//why this ? can not i just use FMT and it handle this ?
// std::string toStdString(const char* c);
// std::string toStdString(const QString& c);
// std::string toStdString(const QByteArray& c);
// std::string toStdString(const std::string& c);
// std::string toStdString(const std::string_view& c);
