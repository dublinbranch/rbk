#pragma once

#include "fwd.h"
#include <string>

namespace bj = boost::json;
void mergeJson(bj::object& target, const bj::object& mixMe, bool overwrite = false);

// subtract the second from the first
bj::object subtractJson(const bj::object& first, const bj::object& second, std::string path = {});

// Keys present in original but absent from expected, as JSON Pointer paths (/key or /parent/key).
#include <string>
#include <vector>

std::vector<std::string> extraJsonKeyPaths(const bj::object& original, const bj::object& expected,
                                           std::string_view basePath = {});
