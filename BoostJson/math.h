#pragma once

#include "fwd.h"
#include <string>

namespace bj = boost::json;
void mergeJson(bj::object& target, const bj::object& mixMe, bool overwrite = false);

//subtract the second from the first
bj::object subtractJson(const bj::object& first, const bj::object& second, std::string path = {});
