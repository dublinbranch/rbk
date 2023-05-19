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

void SqlComposer::push(const SScol& col, bool force) {
	//This is just to showcase the usage of a lamba, in this case a normal for loop would probably have been easier
	auto comp = [&](const SScol& a) {
		if (a.key == col.key) {
			return true;
		}
		return false;
	};

	{
		auto iter = std::find_if(
		    begin(),
		    end(),
		    comp);

		if (iter != end()) {
			if (!force) {
				throw ExceptionV2(F("you are inserting twice the same KEY: {}, current value is {}, previous value was {}\n", col.key, col.val.val, iter->val.val));
			}
			erase(iter);
		}
	}

	push_back(col);
	//in padding and readability we forever trust
	longestKey = std::max(longestKey, col.key.size());
	longestVal = std::max(longestVal, col.val.val.size());
}

std::string SqlComposer::compose() const {
	//In padding we trust

	string final = "\n";
	final.reserve(16000);
	//plain fmt::join is not enought here
	bool first = true;
	for (auto&& [aritmetic, key, val] : *this) {
		string valueS1;
		//no ONE in his right mind will save a string called NULL in a database
		if (aritmetic || val.noEscape || val.noQuote || val.val == "NULL") {
			valueS1 = val.val;
		} else { //no longer we will abuse base64this!
			valueS1 = "'"s + db->escape(val.val) + "'"s;
		}

		//t = F("{:>{}}", t, longestVal);

		const string* kk      = nullptr;
		const string* vv      = nullptr;
		uint          longest = 0;
		if (isASelect) {
			kk      = &valueS1;
			vv      = &key;
			longest = longestVal;
		} else {
			kk      = &key;
			vv      = &valueS1;
			longest = longestKey;
		}

		string keyS1 = F("{:>{}}", *kk, longest);

		if (first) {
			first               = false;
			auto initialPadding = F("{:>{}} ", " ", separator.size());
			final += F("{}{}{}{}\n"s, initialPadding, keyS1, joiner, *vv);
		} else {
			final.append(separator) += F(" {}{}{}\n"s, keyS1, joiner, *vv);
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

string SqlComposer::composeUpdate() const {
	getTable();
	string sql = F(R"(
UPDATE {} SET
{}
WHERE {}
)",
	               table, compose(), where->compose());
	return sql;
}

string SqlComposer::composeInsert() const {
	getTable();
	if (!where->empty()) {
		throw ExceptionV2("Refusing an insert with where condition");
	}
	auto sql = F("INSERT INTO {} SET {}", table, compose());
	return sql;
}

string SqlComposer::composeDelete() const {
	getTable();
	if (where->empty()) {
		throw ExceptionV2("Refusing a delete with no where condition");
	}

	auto sql = F("DELETE FROM {} WHERE {}", table, where->compose());
	return sql;
}

std::string SqlComposer::getTable() const {
	if (table.empty()) {
		throw ExceptionV2("not table set!");
	}
	return table;
}

void SqlComposer::setIsASelect() {
	joiner    = " AS ";
	isASelect = true;
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
