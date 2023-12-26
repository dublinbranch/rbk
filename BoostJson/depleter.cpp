#include "depleter.h"

Depleter::Depleter(boost::json::value* json_) {
	json = json_;
}

Depleter Depleter::operator[](std::string_view key) {
	if (!json->is_object()) {
		throw ExceptionV2("this is not an object!");
	}
	return Depleter(&json->as_object().at(key));
}

Depleter Depleter::operator[](uint pos) {
	if (!json->is_object()) {
		throw ExceptionV2("this is not an array!");
	}
	return Depleter(&json->as_array().at(pos));
}
