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
		SqlRowV2 r;
		r.columns = columns;
		for (const auto& col : row) {
			r.data.push_back(col.second.toStdString());
		}
		//cross check the data are the same
		for (auto& [name, info] : *r.columns) {
			auto o = row.rq<std::string>(QByteArray::fromStdString(name));
			auto n = r.rq(name);
			if (o != n) {
				int x = 0;
				(void)x;
			}
		}

		push_back(r);
	}
}

SqlRowV2::SqlRowV2(const sqlRow& old) {
	columns  = std::make_shared<SqlResV2::TypeMap>();
	uint   i = 0;
	MyType mt(enum_field_types::MAX_NO_FIELD_TYPES);

	for (auto&& [key, value] : old) {
		columns->insert({key.toStdString(), SqlResV2::Field{mt, i}});
		i++;
		data.push_back(value.toStdString());
	}
}

bool SqlRowV2::empty() const {
	return data.empty();
}
