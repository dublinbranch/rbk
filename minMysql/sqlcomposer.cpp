#include "sqlcomposer.h"
#include "min_mysql.h"
#include "rbk/string/util.h"

using namespace std;

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

	string final = "\n";
	final.reserve(16000);
	//plain fmt::join is not enought here
	bool first = true;
	for (auto&& col : *this) {
		string valueS1;
		//no ONE in his right mind will save a string called NULL in a database
		if (col.aritmetic || col.val.noEscape || col.val.noQuote || col.val.val == "NULL") {
			valueS1 = col.val.val;
		} else { //no longer we will abuse base64this!
			valueS1 = "'"s + db->escape(col.val.val) + "'"s;
		}

		string keyS1;
		if (col.verbatim) {
			keyS1 = F("{:>{}}", col.key, longestKey);
		} else {
			auto t = F("`{}`", col.key);
			keyS1  = F("{:>{}}", t, longestKey);
		}

		if (first) {
			first               = false;
			auto initialPadding = F("{:>{}} ", " ", separator.size());
			if (col.verbatim) {
				final += F("{}{}\n"s, initialPadding, keyS1);
			} else {
				final += F("{}{}{}{}\n"s, initialPadding, keyS1, joiner, valueS1);
			}

		} else {
			if (col.verbatim) {
				final.append(separator) += F(" {}\n"s, keyS1);
			} else {
				final.append(separator) += F(" {}{}{}\n"s, keyS1, joiner, valueS1);
			}
		}
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

	auto sql = "SELECT " + fields + composeFrom() + composeWhere();

	return sql;
}

string SqlComposer::composeSelect_V2() {
	getTable();
	setIsASelect();

	auto sql = "SELECT " + compose() + composeFrom() + composeWhere();

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
	string sql = "SELECT * " + composeFrom() + composeWhere();
	return sql;
}

string SqlComposer::composeUpdate() const {
	getTable();
	string sql = F(R"(UPDATE {} SET )", table) + compose() + composeWhere(true) + ";";
	return sql;
}

QString SqlComposer::composeUpdateQS() const {
	return QString::fromStdString(composeUpdate());
}

string SqlComposer::composeUpsert() const {
	getTable();
	if (!where->empty()) {
		throw ExceptionV2("Refusing an Upsert with where condition");
	}
	auto c   = compose();
	auto sql = F("INSERT INTO {} SET {} ON DUPLICATE KEY UPDATE {};", table, c, c);
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
	auto sql = F("INSERT {} INTO {} SET {} ;", ignoreS, table, compose());
	return sql;
}

string SqlComposer::composeDelete() const {
	getTable();
	if (where->empty()) {
		throw ExceptionV2("Refusing a delete with no where condition");
	}

	auto sql = F("DELETE FROM {} WHERE {} ;", table, where->compose());
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
