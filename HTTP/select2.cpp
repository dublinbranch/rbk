#include "select2.h"
#include <boost/json.hpp>

using namespace boost::json;

std::string Select2::Result::toResultJSON() const {
	object obj;                                              // construct an empty object
	obj["pagination"].emplace_object()["more"] = pagination; // insert a double
	array arr;
	for (auto&& row : rows) {
		object r;
		r["id"]   = row.id;
		r["text"] = row.text;
		arr.emplace_back(r);
	}
	obj["results"] = arr;
	return serialize(obj);
}
