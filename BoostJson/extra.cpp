#include "extra.h"
//one off include to compile what is needed and avoid linking external stuff
#include <boost/json/src.hpp>

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlRow.h"
#include <QByteArray>
#include <QString>

using namespace std;
namespace bj = boost::json;
using namespace std::string_literals;

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t) {
	jv = t.toStdString();
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t) {
	jv = t.toStdString();
}

/*
https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html

struct customer
{
    std::uint64_t id;
    std::string name;
    bool late;

    customer() = default;

    customer( std::uint64_t i, const std::string& n, bool l )
        : id( i ), name( n ), late( l ) { }


};

void tag_invoke( const bj::value_from_tag&, bj::value& jv, customer const& c )
{
    // Assign a JSON value
    jv = {
        { "id", c.id },
        { "name", c.name },
        { "late", c.late }
    };
}


        std::vector< customer > customers = {
                customer( 0, "Alison", false ),
                customer( 1, "Bill", false ),
                customer( 3, "Catherine", true ),
                customer( 4, "Doug", false )
         };



        bj::value jv = bj::value_from( customers);
                //So it understood was a vector...
                assert( jv.is_array() );

*/

using namespace boost;
void pretty_print(std::string& os, json::value const& jv, std::string* indent) {
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
		os += std::to_string(jv.get_uint64());
		break;

	case json::kind::int64:
		os += std::to_string(jv.get_int64());
		break;

	case json::kind::double_:
		os += std::to_string(jv.get_double());
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

json::value asNull(const sqlRow& row, std::string_view key) {

	QByteArray k;
	k.setRawData(key.data(), key.size());
	auto v = row.rq<QByteArray>(k);
	if (v == BSQL_NULL) {
		return nullptr;
	}
	return {v.toStdString()};
}

QString QS(const boost::json::string& cry) {
	return QString::fromLatin1(cry.data(), cry.size());
}

QString QS(const boost::json::value* value) {
	if (value->is_null()) {
		return {};
	}
	return QS(value->as_string());
}

QString QS(const boost::json::value& value) {
	if (value.is_null()) {
		return {};
	}
	return QS(value.as_string());
}

bool insertIfNotNull(boost::json::object& target, const sqlRow& row, std::string_view key) {
	QByteArray k;
	k.setRawData(key.data(), key.size());
	auto v = row.rq<QByteArray>(k);
	if (v == BSQL_NULL) {
		return false;
	}
	target[key] = v.toStdString();
	return true;
}

void pushCreate(boost::json::object& value, std::string_view key, const boost::json::value& newValue) {
	if (auto array = value.if_contains(key); array) {
		if (!array->is_array()) {
			throw ExceptionV2(string("this is not an array!").append(key));
		}
		array->as_array().push_back(newValue);
	} else {
		value[key] = bj::array{newValue};
	}
}

JsonRes parseJson(std::string_view json) {
	JsonRes res;

	bj::parser  p;
	std::size_t consumed = p.write_some(json, res.ec);

	if (res.ec) {
		//take a copy only in case of error
		res.raw      = json;
		res.position = consumed;
	} else {
		res.json = p.release();
	}

	return res;
}

JsonRes parseJson(const QByteArray& json) {
	return parseJson(json.toStdString());
}

void sqlEscape(boost::json::object& r, DB* db) {
	for (auto iter = r.begin(); iter != r.end(); iter++) {
		iter->value() = db->escape(string(iter->value().as_string().c_str()));
	}
}

string JsonRes::composeErrorMsg() const {
	if (!position) {
		return "Valid JSON";
	}

	using Pt = string::size_type;
	struct Payload {
		string_view row;
		Pt          start;
		uint        rowNumber;
	};

	std::map<Pt, Payload> newLines;

	{
		Pt   lineStart = 0;
		Pt   lineEnd   = 0;
		uint rowNumber = 0;
		while (true) {
			//never seen row 0
			rowNumber++;
			lineEnd = raw.find("\n", lineStart);
			if (lineEnd == string::npos) {
				break;
			}
			Payload p{string_view(raw).substr(lineStart, lineEnd - lineStart),
			          lineStart,
			          rowNumber};
			newLines.insert({lineEnd, p});
			lineStart = lineEnd + 1;
		}
	}

	Pt          offset    = 0;
	uint        rowNumber = 0;
	string_view showMe;

	if (auto i = newLines.lower_bound(position); i != newLines.end()) {
		//auto il = i->first;
		auto& r   = i->second;
		rowNumber = r.rowNumber;

		auto start = r.start;
		auto end   = i->first;
		//do we have a line before ?
		if (rowNumber > 2) {
			start = (----i)->second.start;
		} else if (rowNumber > 1) {
			start = (--i)->second.start;
		}
		//do we have a line after ?
		//if (rowNumber < newLines.size()) {
		//TODO insert the pointer and than this other part ?
		//end = (++i)->first;
		//}
		auto len = end - start;
		showMe   = string_view(raw).substr(start, len);

		//row do not start at pos 0
		offset = (position - r.start) + 1;
	} else {
		//either a single line or at the last line
		rowNumber = newLines.size();
		if (!rowNumber) {
			rowNumber = 1;
		}

		//show a part of the json to understand the problem, do not go over end of line or before!
		uint start = max(0u, position - 45);
		uint end   = min(Pt(start + 80), raw.size());

		auto len = end - start;

		showMe = string_view(raw).substr(start, len);
		if (rowNumber) { //if NOT on a single big line the json, take last line info
			offset = position - newLines.rbegin()->first;
		} else {
			offset = position - start;
		}
	}

	auto fancy = F("{: >{}}", "^", offset);

	auto msg = F(R"(
Invalid json 
Error {} 
At position {} (line {}:{})
--------------------
{}
{}
--------------------
)",
	             ec.message(),
	             position,
	             rowNumber, offset,
	             showMe,
	             fancy);
	return msg;
}
