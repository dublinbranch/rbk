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
		r["id"]   = row.id;
		r["text"] = row.text;
		if (row.selected) {
			r["selected"] = true;
		}
		if (!row.formatMe.empty()) {
			r["formatMe"] = row.formatMe;
		}
		arr.emplace_back(r);
	}
	obj["results"] = arr;
	return serialize(obj);
}

Select2::Row::Row(const std::string& id_, const std::string& text_, bool sel) {
	id       = id_;
	text     = text_;
	selected = sel;
}

Select2::Row::Row(const QString& id_, const QString& text_, bool sel) {
	id       = id_.toStdString();
	text     = text_.toStdString();
	selected = sel;
}
