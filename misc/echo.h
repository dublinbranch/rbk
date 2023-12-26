#pragma once

#include <string>
#include <string_view>

class QByteArray;

void echo(const std::string& s);
void echo(const std::string_view s);
void echo(const QByteArray& s);
void warn(const std::string& msg);
void critical(const std::string& msg);
