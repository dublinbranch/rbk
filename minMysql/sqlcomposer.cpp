#include "sqlcomposer.h"

QString SScol::assemble(int padding) const {
	QString final = key.leftJustified(padding) + QSL("= ");
	if (aritmetic) {
		return final + val;
	} else {
		return final + mayBeBase64(val);
	}
}

void SqlComposer::push(const SScol& col) {
	//This is just to showcase the usage of a lamba, in this case a normal for loop would probably have been easier
	auto comp = [&](const SScol& a) {
		if (a.getKey() == col.getKey()) {
			return true;
		}
		return false;
	};

	auto iter = std::find_if(
	    vector.begin(),
	    vector.end(),
	    comp);

	if (iter != vector.end()) {
		throw QSL("you are inserting twice the same KEY: %1, current value is %2, previous value was %3\n").arg(col.getKey(), col.getVal(), iter->getVal());
	}

	vector.push_back(col);
	auto s     = col.getKey().size();
	longestKey = std::max(longestKey, s);
}

QString SqlComposer::compose() const {
	//In padding we trust

	QString final;
	final.reserve(16000);
	for (auto iter = vector.begin(); iter != vector.end() - 1; ++iter) {
		final += iter->assemble(longestKey + 1) + QSL(",\n");
	}
	final += vector.back().assemble(longestKey + 1) + QSL("\n");
	return final;
}

QString SScol::getKey() const {
	return key;
}

QString SScol::getVal() const {
	return val;
}
