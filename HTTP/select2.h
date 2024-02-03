#ifndef SELECT2_H
#define SELECT2_H

#include <boost/json/value.hpp>
#include <string>
#include <vector>

class QString;
class PMFCGI;
class sqlResult;
class Payload;
class sqlRow;
class QStringAdt;

namespace Select2 {
/*
 {
  "results": [
    {
      "id": 1,
      "text": "Option 1",
          "formatMe" : {JSON} or "" depending on the receiving function
    },
    {
      "id": 2,
      "text": "Option 2"
    }
  ],
  "pagination": {
    "more": true
  }
}
*/

struct Row {
	Row() = default;
	Row(const std::string& id_, const std::string& text_, bool sel = false);
	Row(const QString& id_, const QString& text_, bool sel = false);
	std::string         id;
	std::string         text;
	boost::json::object formatMe;
	bool                selected = false;
};

struct Result {
	bool             pagination = false;
	std::vector<Row> rows;
	std::string      toResultJSON() const;
};

struct PkConf {
	//do not waste time using typedef or decltype just learn functor sintax
	using StrReplaceFuntor     = void (*)(std::string& txt);
	using StringAssemblyFuntor = Select2::Row(const sqlRow& txt);

	//to replace a single string block (normally to place &nbsp; instead of space
	StrReplaceFuntor strReplace = nullptr;

	//to assemble it based on the result (std::function use to pass lambda)
	std::function<StringAssemblyFuntor> stringAssembly = nullptr;

	//add at the end a NONE row, with ID = NULL, used to unset the dropdown
	bool NONE = false;
};

enum class SearchPattern {
	START,
	ANYWHERE,
	EXACT,
};

std::string search(const QStringAdt& http, std::string_view sql, PMFCGI& status, SearchPattern pattern = SearchPattern::START);
std::string limits(PMFCGI& status);
void        packer2(const sqlResult& rows, Payload& payload, PkConf* pkConf = nullptr);

} // namespace Select2

#endif // SELECT2_H
