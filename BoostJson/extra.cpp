#include "extra.h"
//one off include to compile what is needed and avoid linking external stuff
#include <boost/json/src.hpp>

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlRow.h"
#include <QByteArray>
#include <QString>

#include "rbk/QStacker/httpexception.h"

using namespace std;
namespace bj = boost::json;
using namespace std::string_literals;

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
		os += F("{}"sv, jv.get_uint64());
		break;

	case json::kind::int64:
		os += F("{}"sv, jv.get_int64());
		break;

	case json::kind::double_:
		os += F("{}"sv, jv.get_double());
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
	k.setRawData(key.data(), static_cast<uint>(key.size()));
	auto v = row.rq<QByteArray>(k);
	if (v == BSQL_NULL) {
		return nullptr;
	}
	return {v.toStdString()};
}

QString QS(const json::string_view& cry) {
#if QT_VERSION_MAJOR == 5
	return QString::fromUtf8(cry.data(), (int)cry.size());
#elif QT_VERSION_MAJOR == 6
	return QString::fromUtf8(cry.data(), cry.size());
#endif
}

QString QS(const boost::json::string& cry) {
#if QT_VERSION_MAJOR == 5
	return QString::fromUtf8(cry.data(), (int)cry.size());
#elif QT_VERSION_MAJOR == 6
	return QString::fromUtf8(cry.data(), cry.size());
#endif
}

QByteArray QB(const boost::json::string& cry) {
#if QT_VERSION_MAJOR == 5
	auto q = QByteArray::fromRawData(cry.data(), (int)cry.size());
	q.detach();
	return q;
#elif QT_VERSION_MAJOR == 6
	auto q = QByteArray::fromRawData(cry.data(), cry.size());
	q.detach();
	return q;
#endif
}

QByteArray QB(const boost::json::string_view& cry) {
#if QT_VERSION_MAJOR == 5
	auto q = QByteArray::fromRawData(cry.data(), (int)cry.size());
	q.detach();
	return q;
#elif QT_VERSION_MAJOR == 6
	auto q = QByteArray::fromRawData(cry.data(), cry.size());
	q.detach();
	return q;
#endif
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
	k.setRawData(key.data(), static_cast<uint>(key.size()));
	auto v = row.rq<QByteArray>(k);
	if (v.isEmpty() || v == BSQL_NULL) {
		return false;
	}
	target[key] = v.toStdString();
	return true;
}

void pushCreate(boost::json::object& json, std::string_view key, const boost::json::value& newValue) {
	if (auto array = json.if_contains(key); array) {
		if (!array->is_array()) {
			throw ExceptionV2(string("this is not an array!").append(key));
		}
		array->as_array().push_back(newValue);
	} else {
		json[key] = bj::array{newValue};
	}
}

//TODO pass a context to support auto trowh and print in QCritical ecc
JsonRes parseJson(std::string_view json, bool throwOnError) {
	JsonRes res;

	if (json.empty()) {
		if (throwOnError) {
			throw ExceptionV2("Empty JSON");
		}
		return res;
	}

	bj::parse_options opt;            // all extensions default to off
	opt.allow_comments        = true; // permit C and C++ style comments to appear in whitespace
	opt.allow_trailing_commas = true; // allow an additional trailing comma in object and array element lists

	//value jv = parse("[1,2,3,] // comment ", storage_ptr(), opt);
	bj::parser  p(res.storage, opt);
	std::size_t consumed = p.write_some(json, res.ec);

	if (res.ec) {
		//take a copy only in case of error
		res.raw      = json;
		res.position = consumed;
		if (throwOnError) {
			throw HttpException(res.composeErrorMsg());
		}
	} else {
		res.json = p.release();
	}

	return res;
}

JsonRes parseJson(const std::string& json, bool throwOnError) {
	return parseJson(string_view(json), throwOnError);
}

JsonRes parseJson(const QByteAdt& json, bool throwOnError) {
	return parseJson(json.toStdString(), throwOnError);
}

void sqlEscape(boost::json::object& r, DB* db) {
	for (auto iter = r.begin(); iter != r.end(); iter++) {
		iter->value() = db->escape(string(iter->value().as_string().c_str()));
	}
}

string JsonRes::composeErrorMsg() const {
	if (raw.empty()) {
		return "Empty JSON";
	}
	if (!position) {
		return "Invalid JSON";
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
	size_t      rowNumber = 0;
	string_view showMe;

	if (auto i = newLines.lower_bound(position); i != newLines.end()) {
		//auto il = i->first;
		auto& r   = i->second;
		rowNumber = r.rowNumber;

		auto start = r.start;
		auto end   = i->first;
		//do we have a line before ?
		if (rowNumber > 2) {
			start = (-- --i)->second.start;
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
		size_t zero  = 0;
		auto   start = max(zero, position - 45);
		auto   end   = min(Pt(start + 80), raw.size());

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
At line {}:{} (byte from start {})
--------------------
{}
{}
--------------------
)",
	             ec.message(),
	             rowNumber, offset,
	             position,
	             showMe,
	             fancy);
	return msg;
}

QString serializeQS(const boost::json::value& jv) {
	auto t = pretty_printQS(jv);
	return t;
}

QString pretty_printQS(const boost::json::value& jv) {
	return QString::fromStdString(pretty_print(jv));
}

#include <iomanip>
#include <sstream>
//https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
std::string escape_json(const std::string& s) {
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++) {
		switch (*c) {
		case '"':
			o << "\\\"";
			break;
		case '\\':
			o << "\\\\";
			break;
		case '\b':
			o << "\\b";
			break;
		case '\f':
			o << "\\f";
			break;
		case '\n':
			o << "\\n";
			break;
		case '\r':
			o << "\\r";
			break;
		case '\t':
			o << "\\t";
			break;
		default:
			if (*c <= '\x1f') {
				o << "\\u"
				  << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
			} else {
				o << *c;
			}
		}
	}
	return o.str();
}

QString escape_json(const QString& string) {
	return QString::fromStdString(escape_json(string.toStdString()));
}

void createOrAppendObj(boost::json::object& json, std::string_view container, std::string_view newElement, const boost::json::value& newValue) {
	if (auto* obj = json.if_contains(container); obj) {
		obj->as_object()[newElement] = newValue;
	} else {
		json[container] = {{newElement, newValue}};
	}
}

string_view asString(const boost::json::value& value) {
	auto& r = value.as_string();
	return std::string_view(r.data(), r.size());
}

QString QS(const boost::json::value& value, std::string_view key, const QString& def) {
	if (auto el = value.as_object().if_contains(key); el) {
		return QS(*el);
	}

	return def;
}

QByteArray QB(const boost::json::value& value) {
	if (value.is_null()) {
		return {};
	}
	return QB(value.as_string());
}

QByteArray QB(const boost::json::value& value, std::string_view key, const QByteArray& def) {
	if (auto el = value.as_object().if_contains(key); el) {
		return QB(*el);
	}

	return def;
}

std::string_view asString(const boost::json::object& value, std::string_view key, std::string_view def) {
	if (auto el = value.if_contains(key); el) {
		return asString(*el);
	}
	return def;
}

std::string_view asString(const boost::json::object& value, StringAdt key) {
	return asString(value, string_view(key));
}

std::string_view asString(const boost::json::object& value, std::string_view key) {
	if (auto el = value.if_contains(key); el) {
		return asString(*el);
	}
	return {};
}

std::string_view asStringThrow(const boost::json::object& value, std::string_view key) {
	if (auto el = value.if_contains(key); el) {
		if (!el->is_string()) {
			throw ExceptionV2(F("{} is not a string", key));
		}
		return asString(*el);
	}
	throw ExceptionV2(F("Impossible to find {} in json", key));
}

std::string_view SW(const boost::json::string_view& cry) {
	return std::string_view(cry);
}

std::string_view asString(const boost::json::object& value, const char* key) {
	return asString(value, string_view(key));
}

string missingKeyError(std::string_view key, bool noThrow) {
	auto msg = F("impossible to find elment {}", key);
	if (noThrow) {
		return msg;
	}
	throw ExceptionV2(msg);
}

std::string_view asString(const boost::json::value& value, std::string_view key) {
	return asString(value.as_object(), key);
}

std::expected<std::string_view, string> asStringVerbose(const boost::json::object& value, std::string_view key) {
	if (auto el = value.if_contains(key); el) {
		if (!el->is_string()) {
			return std::unexpected(F("{} is not a string", key));
		}
		return asString(*el);
	}
	return std::unexpected(F("Impossible to find {} in json", key));
}

string join(const boost::json::array& array) {
	return bj::serialize(array);
}

void swap(const boost::json::value& el, std::vector<string>& target) {
	target.clear();

	if (el.is_array()) {
		auto& arr = el.as_array();
		target.reserve(arr.size());
		for (const auto& item : arr) {
			if (item.is_string()) {
				target.push_back(to_string(item.as_string()));
			} else {
				throw ExceptionV2(F("something is not a string but a {} = {}", el.kind(), pretty_print(el)));
			}
			target.push_back(to_string(item.as_string()));
		}
	} else if (el.is_string()) {
		target.push_back(to_string(el.as_string()));
	} else {
		throw ExceptionV2(F("something is not a string but a {} = {}", el.kind(), pretty_print(el)));
	}
};

std::vector<string> toVecString(const bj::value& v) {
	std::vector<string> res;
	swap(v, res);
	return res;
}

std::pair<bool, std::string_view> delete_at_pointer(std::string_view sv, bj::value* value) {
	system::error_code ec;

	string_view     previous_segment;
	bj::string_view sv_copy = sv;
	string_view     err_position;
	string_view     segment = bj::detail::next_segment(sv_copy, ec);
	size_t          shift   = 0;

	auto result          = value;
	auto previous_result = value;

	while (true) {
		if (ec.failed())
			return {false, err_position};

		if (!result) {
			return {false, err_position};
		}

		if (segment.empty())
			break;

		shift += segment.size();
		err_position = sv_copy.substr(0, shift);

		previous_segment = segment;
		previous_result  = result;

		switch (result->kind()) {
		case bj::kind::object: {
			auto& obj = result->get_object();

			bj::detail::pointer_token const token(segment);
			segment = bj::detail::next_segment(sv_copy, ec);

			result = bj::detail::if_contains_token(obj, token);
			if (!result) {
				return {false, err_position};
			}
			break;
		}
		case bj::kind::array: {
			auto const index = bj::detail::parse_number_token(segment, ec);
			segment          = bj::detail::next_segment(sv_copy, ec);

			auto& arr = result->get_array();
			result    = arr.if_contains(index);
			if (!result) {
				return {false, err_position};
			}
			break;
		}
		default: {
			return {false, err_position};
		}
		}
	}

	err_position = {};

	switch (previous_result->kind()) {
	case bj::kind::object: {
		auto&                           obj = previous_result->get_object();
		bj::detail::pointer_token const token(previous_segment);
		bj::key_value_pair*             kv = bj::detail::find_in_object(obj, token).first;
		if (kv) {
			obj.erase(kv);
			return {true, err_position};
		}
		break;
	}
	case bj::kind::array: {
		auto const index = bj::detail::parse_number_token(previous_segment, ec);
		auto&      arr   = previous_result->get_array();
		if (arr.if_contains(index)) {
			arr.erase(arr.begin() + index);
			return {true, err_position};
		}
		break;
	}
	default: {
		return {false, err_position};
	}
	}
	return {false, err_position};
}
