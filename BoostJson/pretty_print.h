#pragma once

// Standalone: Boost.JSON + std only. Do not pull extra.h (Qt, mysql, fmt wrappers).
#include <boost/json/value.hpp>
#include <string>

void        pretty_print(std::string& os, boost::json::value const& jv, std::string* indent = nullptr);
std::string pretty_print(boost::json::value const& jv);
