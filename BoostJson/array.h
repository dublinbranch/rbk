#pragma once

#include <boost/json.hpp>
#include <string>
#include <vector>

//JUST use json["range"] = bj::value_from(range);
// Templated function to convert std::vector to boost::json::array
// template <typename T>
// boost::json::array
// vector_2_jarray(const std::vector<T>& vec) {
// 	boost::json::array jsonArray;
// 	for (const auto& element : vec) {
// 		jsonArray.push_back(boost::json::value_from(element));
// 	}
// 	return jsonArray;
// }

