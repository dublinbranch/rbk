#ifndef CSV_H
#define CSV_H

#include <string>
#include <string_view>

std::string escapeForCSV(const std::string_view& input);

#endif // CSV_H
