#pragma once

#include <string>
#include <string_view>
void echo(const std::string& s);
void echo(const std::string_view s);
void warn(std::string& msg);
void critical(std::string& msg);

