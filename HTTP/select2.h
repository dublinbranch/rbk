#ifndef SELECT2_H
#define SELECT2_H

#include <boost/json/value.hpp>
#include <string>
#include <vector>

class QString;

namespace Select2 {
/*
 {
  "results": [
    {
      "id": 1,
      "text": "Option 1",
          "formatMe" : {} or "" depending on the receiving function
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

} // namespace Select2

#endif // SELECT2_H
