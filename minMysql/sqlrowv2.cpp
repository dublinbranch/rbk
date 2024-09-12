#include "sqlrowv2.h"

SqlResultV2::SqlResultV2() {
	columns = std::make_shared<SqlResV2::TypeMap>();
}

SqlResultV2::SqlResultV2(const sqlResult& old) {
	// Copy columns
	columns              = std::make_shared<SqlResV2::TypeMap>();
	const auto& firstRow = old.begin();
	uint        i        = 0;
	MyType      mt(enum_field_types::MAX_NO_FIELD_TYPES);

	for (auto&& [key, value] : *firstRow) {
		columns->insert({key.toStdString(), SqlResV2::Field{mt, i}});
		i++;
	}
	// Copy rows
	for (auto& row : old) {
		for (const auto& col : row) {
			SqlRowV2 r;
			r.data.push_back(col.second.toStdString());
			push_back(r);
		}
	}
}

bool SqlRowV2::empty() const {
	return data.empty();
}
