#pragma once

#include "rbk/string/stringoso.h"
#include <string>
#include <string_view>

void echo(const StringViewV2& s);

void warn(const std::string& msg);
void critical(const std::string& msg);
