#pragma once
#include <boost/json.hpp>
#include <optional>

std::optional<boost::json::value> getValueFromNonsense(const boost::json::array& nonsense, std::string_view keyName, std::string_view keyValue, std::string_view valueName);
