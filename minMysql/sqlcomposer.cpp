#include "sqlcomposer.h"
#include "min_mysql.h"
#include "rbk/string/util.h"

using namespace std;

SqlComposer::SqlComposer(DB* db_, const std::string& separator_) {
	db        = db_;
	separator = separator_;
	replace(" ", "", separator);
}

void SqlComposer::push(const SScol& col) {
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
			throw ExceptionV2(F("you are inserting twice the same KEY: {}, current value is {}, previous value was {}\n", col.key, col.val, iter->val));
		}
	}

	push_back(col);
	//in padding and readability we forever trust
	longestKey = std::max(longestKey, col.key.size());
	longestVal = std::max(longestVal, col.val.size());
}

std::string SqlComposer::compose() const {
	//In padding we trust

	string final = "\n";
	final.reserve(16000);
	//plain fmt::join is not enought here
	bool first = true;
	for (auto&& [math, key, val] : *this) {
		string t;
		if (math) {
			t = val;
		} else {
			t = "'"s + db->escape(val) + "'"s;
		}
		//t = F("{:>{}}", t, longestVal);

		if (first) {
			first = false;
			//the first block is used for padding based on separator lenght
			final += F("{:>{}} {:>{}} = {} \n"s, " ", separator.size(), key, longestKey, t);
		} else {
			final.append(separator) += F(" {:>{}} = {} \n"s, key, longestKey, t);
		}
	}
	return final;
}
//	for (auto iter = vector.begin(); iter != vector.end() - 1; ++iter) {
//		final += iter->assemble(longestKey + 1) + QSL(",\n");
//	}
//	final += vector.back().assemble(longestKey + 1) + QSL("\n");
//	return final;
