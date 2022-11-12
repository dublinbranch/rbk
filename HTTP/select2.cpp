#include "select2.h"
#include <QString>
#include <boost/json.hpp>

using namespace boost::json;

using namespace std::string_literals;

std::string Select2::Result::toResultJSON() const {
	object obj;
	obj["pagination"].emplace_object()["more"] = pagination;
	array arr;
	for (auto&& row : rows) {
		object r;
		r["id"s]  = row.id;
		r["text"s] = row.text;
		arr.emplace_back(r);
	}
	obj["results"] = arr;
	return serialize(obj);
}

Select2::Row::Row(const std::string& id_, const std::string& text_) {
	id   = id_;
	text = text_;
}

Select2::Row::Row(const QString& id_, const QString& text_) {
	id   = id_.toStdString();
	text = text_.toStdString();
}
