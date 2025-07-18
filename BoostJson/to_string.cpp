#include "to_string.h"
#include "rbk/fmtExtra/dynamic.h"
#include <boost/json.hpp>

namespace json = boost::json;

std::string to_string(const boost::json::value* jv) {
	return to_string(*jv);
}

std::string to_string(const json::value& jv) {
	std::string val;
	switch (jv.kind()) {
	case json::kind::string: {
		val = jv.get_string();
		break;
	}

	case json::kind::uint64:
		val = F("{}", jv.get_uint64());
		break;

	case json::kind::int64:
		val = F("{}", jv.get_uint64());
		break;

	case json::kind::double_:
		val = F("{}", jv.get_uint64());
		break;

	case json::kind::bool_:
		if (jv.get_bool()) {
			val = "true";
		} else {
			val = "false";
		}
		break;

	case json::kind::null:
		val = "null";
		break;
	default:
		throw std::runtime_error("this can not be used on array or object");
	}
	return val;
}
