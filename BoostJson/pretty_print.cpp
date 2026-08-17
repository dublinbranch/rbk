#include "pretty_print.h"

#include <boost/json.hpp>

void pretty_print(std::string& os, boost::json::value const& jv, std::string* indent) {
	namespace json = boost::json;

	std::string indent_;
	if (!indent)
		indent = &indent_;
	switch (jv.kind()) {
	case json::kind::object: {
		os += "{\n";
		indent->append(4, ' ');
		auto const& obj = jv.get_object();
		if (!obj.empty()) {
			auto it = obj.begin();
			for (;;) {
				os += *indent + json::serialize(it->key()) + " : ";
				pretty_print(os, it->value(), indent);
				if (++it == obj.end())
					break;
				os += ",\n";
			}
		}
		os += "\n";
		indent->resize(indent->size() - 4);
		os += *indent + "}";
		break;
	}

	case json::kind::array: {
		os += "[\n";
		indent->append(4, ' ');
		auto const& arr = jv.get_array();
		if (!arr.empty()) {
			auto it = arr.begin();
			for (;;) {
				os += *indent;
				pretty_print(os, *it, indent);
				if (++it == arr.end())
					break;
				os += ",\n";
			}
		}
		os += "\n";
		indent->resize(indent->size() - 4);
		os += *indent + "]";
		break;
	}

	case json::kind::string: {
		os += json::serialize(jv.get_string());
		break;
	}

	case json::kind::uint64:
	case json::kind::int64:
	case json::kind::double_:
		os += json::serialize(jv);
		break;

	case json::kind::bool_:
		if (jv.get_bool()) {
			os += "true";
		} else {
			os += "false";
		}
		break;

	case json::kind::null:
		os += "null";
		break;
	}

	if (indent->empty()) {
		os += "\n";
	}
}

std::string pretty_print(const boost::json::value& jv) {
	std::string res;
	pretty_print(res, jv);
	return res;
}
