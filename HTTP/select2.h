#ifndef SELECT2_H
#define SELECT2_H

#include <string>
#include <vector>

class QString;

namespace Select2 {
/*
 {
  "results": [
    {
      "id": 1,
      "text": "Option 1"
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
	Row(const std::string& id_, const std::string& text_, bool sel = false);
	Row(const QString& id_, const QString& text_, bool sel = false);
	std::string id;
	std::string text;
	bool        selected = false;
};

struct Result {
	bool             pagination = false;
	std::vector<Row> rows;
	std::string      toResultJSON() const;
};

} // namespace Select2

#endif // SELECT2_H
