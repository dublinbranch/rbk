#ifndef SELECT2_H
#define SELECT2_H

#include <string>
#include <vector>

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
	std::string id;
	std::string text;
};

struct Result {
	bool             pagination;
	std::vector<Row> rows;
	std::string toResultJSON() const;
};


} // namespace Select2

#endif // SELECT2_H
