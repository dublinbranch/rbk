#include "sqlrowv2.h"
#include "min_mysql.h"
#include "sqlcomposer.h"

SqlResultV2::SqlResultV2() {
	columns = std::make_shared<SqlResV2::TypeMap>();
}

SqlResultV2::SqlResultV2(const sqlResult& old) {
	if (old.empty()) {
		return;
	}
	// Copy columns
	columns              = std::make_shared<SqlResV2::TypeMap>();
	const auto& firstRow = old.begin();
	uint        i        = 0;

	for (auto&& [key, value] : *firstRow) {
		MyType mt(enum_field_types::MAX_NO_FIELD_TYPES);

		old.types.get(key, mt);

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

QDataStream& operator<<(QDataStream& out, const SqlResultV2& result) {
	// Serialize QVector<SqlRowV2> (inherited part)
	const QVector<SqlRowV2>& baseRef = result; // Get reference to base class
	out << baseRef;
	out << *result.columns;
	return out;
}

QDataStream& operator>>(QDataStream& in, SqlResultV2& result) {
	// Deserialize QVector<SqlRowV2> (inherited part)
	QVector<SqlRowV2>& baseRef = result; // Get reference to base class
	in >> baseRef;                       // Deserialize directly into base class reference

	// Deserialize additional members
	in >> *result.columns;

	//inject in all row the columns
	for (auto& row : baseRef) {
		row.columns = result.columns;
	}

	result.fromCache = true;

	return in;
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

std::string SqlRowV2::prettyPrint(DB* db) const {
	SqlComposer s(db);
	for (auto& [key, info] : *columns) {
		s.push(key, data[info.pos]);
	}
	return s.compose();
}

QDataStream& operator<<(QDataStream& out, const SqlRowV2& row) {
	out << row.data;
	return out;
}

QDataStream& operator>>(QDataStream& in, SqlRowV2& row) {
	// Clear the map first to prepare for deserialization
	row.data.clear();
	in >> row.data;
	return in;
}

QDataStream& operator<<(QDataStream& out, const SqlResV2::Field& field) {
	// Serialize each member
	out << field.type;
	out << field.pos;
	return out;
}

QDataStream& operator>>(QDataStream& in, SqlResV2::Field& field) {
	// Deserialize each member
	in >> field.type;
	in >> field.pos;
	return in;
}
