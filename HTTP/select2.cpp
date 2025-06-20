#include "select2.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"
#include <QString>
#include <boost/json.hpp>

namespace bj = boost::json;

using namespace std::string_literals;
using namespace std;

std::string
Select2::Result::toResultJSON() const {
	bj::object obj;
	obj["pagination"].emplace_object()["more"] = pagination;
	bj::array arr;
	for (auto&& row : rows) {
		bj::object r;
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

std::string Select2::search(DB* db, const QStringAdt& http, std::string_view sql, PMFCGI& status, SearchPattern pattern) {
	string search;
	if (auto v = status.get.get(http); v) {
		switch (pattern) {
		case SearchPattern::START:
			search = F(" `{}` LIKE '{}%'", sql, db->escape(*v.val));
			break;
		case SearchPattern::ANYWHERE:
			search = F(" `{}` LIKE '%{}%'", sql, db->escape(*v.val));
			break;
		case SearchPattern::EXACT:
			search = F(" `{}` = '{}'", sql, db->escape(*v.val));
			break;
		}
	}
	return search;
}

string Select2::limits(PMFCGI& status) {
	if (auto page = status.get.get("page"); !page) {
		return " LIMIT 50";
	} else {
		//Looks like some version are ignorant and start to count from 1 and not 0 -.- so we have to compensate for they ignorance!
		auto p = page.val->toUInt() - 1;
		if (p == 0) {
			return " LIMIT 50";
		}
		auto offset = 50 * p;
		return F(" LIMIT 50 OFFSET {}", offset);
	}
}

void Select2::packer2(const sqlResult& rows, Payload& payload, PkConf* pkConf) {
	Select2::Result res;
	res.pagination = false;
	for (auto&& row : rows) {
		if (pkConf && pkConf->stringAssembly) {
			res.rows.emplace_back(pkConf->stringAssembly(row));
		} else {
			string name = row.rq<string>("name");
			if (pkConf && pkConf->strReplace) {
				pkConf->strReplace(name);
			}
			res.rows.emplace_back(row.rq<string>("id"), name, row.contains("selected"));
		}
	}
	if (pkConf && pkConf->NONE) {
		res.rows.emplace_back("NULL"s, "NONE"s);
	}
	payload.html = res.toResultJSON();
}

void Select2::packer2(const SqlResultV2& rows, Payload& payload) {
	Select2::Result res;
	res.pagination = false;
	for (auto&& row : rows) {
		res.rows.emplace_back(row.rq<string>("id"), row.rq<string>("name"));
	}
	payload.html = res.toResultJSON();
}
