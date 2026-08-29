#include "sqlcomposer.h"
#include "min_mysql.h"
#include "rbk/string/util.h"

using namespace std;

namespace {

void appendPaddedKey(string& out, const SScol& col, u64 longestKey) {
	const auto contentLen = col.verbatim ? col.key.size() : col.key.size() + 2;
	if (contentLen < longestKey) {
		out.append(longestKey - contentLen, ' ');
	}
	if (col.verbatim) {
		out.append(col.key);
	} else {
		out.push_back('`');
		out.append(col.key);
		out.push_back('`');
	}
}

void appendValue(string& out, const SScol& col, DB* db) {
	//no ONE in his right mind will save a string called NULL in a database
	if (col.aritmetic || col.val.noEscape || col.val.noQuote || col.val.val == "NULL") {
		out.append(col.val.val);
	} else {
		out.push_back('\'');
		out.append(db->escape(col.val.val));
		out.push_back('\'');
	}
}

} // namespace

SqlComposer::SqlComposer(DB* db_, const std::string& separator_) {
	db        = db_;
	separator = separator_;
	replace(" ", "", separator);
	PrivateTag t;
	where            = make_unique<SqlComposer>(t);
	where->db        = db;
	where->separator = " AND ";
}

void SqlComposer::push(const SScol& col, bool replaceIf) {
	if (auto iter = findByKey(col.key); iter != this->end()) {
		if (iter->key == col.key) {
			if (replaceIf) {
				erase(iter);
			} else {
				throw ExceptionV2(F("you are inserting twice the same KEY: {}, current value is {}, previous value was {}\n", col.key, col.val.val, iter->val.val));
			}
		}
	}
	pushNoCheck(col);
}

void SqlComposer::pushNoCheck(const SScol& col) {
	push_back(col);
	//in padding and readability we forever trust
	longestKey = std::max(longestKey, col.key.size());
	longestVal = std::max(longestVal, col.val.val.size());
}

SqlComposer& SqlComposer::pushRaw(std::string_view raw_) {
	SScol col;
	col.verbatim = true;
	col.key      = raw_;
	push(col);
	return *this;
}

std::string SqlComposer::compose() const {
	//In padding we trust

	string final;
	final.reserve(16000);
	final.push_back('\n');
	bool first = true;
	for (auto&& col : *this) {
		if (first) {
			first = false;
			final.append(separator.size() + 1, ' ');
		} else {
			final.append(separator);
			final.push_back(' ');
		}
		appendPaddedKey(final, col, longestKey);
		if (!col.verbatim) {
			final.append(joiner);
			appendValue(final, col, db);
		}
		final.push_back('\n');
	}

	return final;
}

QString SqlComposer::composeQS() const {
	return QString::fromStdString(compose());
}

string SqlComposer::composeSelect() {
	setIsASelect();
	return compose();
}

string SqlComposer::composeSelect(const std::string& fields) {
	getTable();
	setIsASelect();

	auto sql = "SELECT " + fields + composeFrom() + composeWhere() + join;

	return sql;
}

string SqlComposer::composeSelect_V2() {
	getTable();
	setIsASelect();

	auto sql = "SELECT " + compose() + composeFrom() + join + composeWhere();

	return sql;
}

string SqlComposer::composeWhere(bool required) const {
	if (required && where->empty()) {
		throw ExceptionV2("Nervously refusing an update without where condition (I basically saved the day from overwriting a whole table...)");
	}
	if (!where->empty()) {
		return " WHERE " + where->compose();
	}
	return {};
}

string SqlComposer::composeFrom() const {
	if (table.empty()) {
		throw ExceptionV2("no table set!");
	}

	return " FROM " + table;
}

string SqlComposer::composeSelectAll() {
	getTable();
	string sql = "SELECT * " + composeFrom() + composeWhere() + join;
	return sql;
}

string SqlComposer::composeUpdate() const {
	getTable();
	auto   body = compose();
	auto   wh   = composeWhere(true);
	string sql;
	sql.reserve(8 + table.size() + 5 + body.size() + wh.size() + 1);
	sql.append("UPDATE ");
	sql.append(table);
	sql.append(" SET ");
	sql.append(body);
	sql.append(wh);
	sql.push_back(';');
	return sql;
}

QString SqlComposer::composeUpdateQS() const {
	return QString::fromStdString(composeUpdate());
}

string SqlComposer::composeUpsert(bool autoInc) const {
	getTable();
	if (!where->empty()) {
		throw ExceptionV2("Refusing an Upsert with where condition");
	}
	auto   c = compose();
	string inc;
	if (autoInc) {
		inc = " ,id = LAST_INSERT_ID(id) ";
	}
	string sql;
	sql.reserve(12 + table.size() + 5 + c.size() + 26 + c.size() + 1 + inc.size() + 1);
	sql.append("INSERT INTO ");
	sql.append(table);
	sql.append(" SET ");
	sql.append(c);
	sql.append(" ON DUPLICATE KEY UPDATE ");
	sql.append(c);
	sql.push_back(' ');
	sql.append(inc);
	sql.push_back(';');
	return sql;
}

string SqlComposer::composeInsert(bool ignora) const {
	getTable();
	if (!where->empty()) {
		throw ExceptionV2("Refusing an insert with where condition");
	}
	string ignoreS;
	if (ignora) {
		ignoreS = " IGNORE ";
	}
	auto   body = compose();
	string sql;
	sql.reserve(7 + ignoreS.size() + 6 + table.size() + 5 + body.size() + 2);
	sql.append("INSERT ");
	sql.append(ignoreS);
	sql.append(" INTO ");
	sql.append(table);
	sql.append(" SET ");
	sql.append(body);
	sql.append(" ;");
	return sql;
}

string SqlComposer::composeDelete() const {
	getTable();
	if (where->empty()) {
		throw ExceptionV2("Refusing a delete with no where condition");
	}

	auto   wh = where->compose();
	string sql;
	sql.reserve(12 + table.size() + 7 + wh.size() + 2);
	sql.append("DELETE FROM ");
	sql.append(table);
	sql.append(" WHERE ");
	sql.append(wh);
	sql.append(" ;");
	return sql;
}

std::string SqlComposer::getTable() const {
	if (table.empty()) {
		throw ExceptionV2("not table set!");
	}
	return table;
}

void SqlComposer::setIsASelect() {
	joiner = " AS ";
}
//	for (auto iter = vector.begin(); iter != vector.end() - 1; ++iter) {
//		final += iter->assemble(longestKey + 1) + QSL(",\n");
//	}
//	final += vector.back().assemble(longestKey + 1) + QSL("\n");
//	return final;

SScol::Value::Value(const std::string& s, bool noQuote_, bool noEscape_) {
	val      = s;
	noQuote  = noQuote_;
	noEscape = noEscape_;
}

SScol::SScol(const std::string& key_) {
	key = key_;
	setVal(key_);
}

SScol::SScol(const char* key_) : SScol(std::string(key_)) {}
