#pragma once

#include <QString>
#include <string>

bool isValidUTF8(const std::string& string, QString* target = nullptr);
bool isValidUTF8(std::string_view string, QString* target = nullptr);
