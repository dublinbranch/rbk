#include "checkschema.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/hash/sha.h"
#include "rbk/minMysql/min_mysql.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>

void removeAutoInc(QString& sql) {
	static QRegularExpression regex(R"RX(AUTO_INCREMENT=(\d*))RX");
	sql.replace(regex, QString());
}

CheckSchema::Schemas CheckSchema::getDbSchema() {
	CheckSchema::Schemas schemas;
	for (auto& dbName : databases) {
		{
			{
				//get all tables in the db and check if we support them (there can be sequence or other stuff we do not handle
				auto sqlTables = F("SELECT * FROM information_schema.`TABLES` WHERE `TABLE_SCHEMA` = '{}'", dbName);
				auto res       = db->query(sqlTables);
				for (auto& row : res) {
					auto        tableName = row.rq("TABLE_NAME");
					auto        type      = row.rq("TABLE_TYPE");
					std::string sqlInfo;
					if (type != "BASE TABLE" && type != "VIEW") {
						throw ExceptionV2(F("Unhandled table type {} in {}", type, dbName, tableName));
					}
				}
			}
			{
				//get all tables in the db at once o.O
				auto sqlInfo = F(R"(
SELECT * 
FROM information_schema.columns 
WHERE table_schema='{}'
ORDER BY `ORDINAL_POSITION` ASC)",
				                 dbName);
				auto res     = db->query(sqlInfo);
				for (auto& row : res) {
					auto tableName = row.rq("TABLE_NAME");
					schemas[{dbName, tableName}].push_back(row);
				}
			}
			{
				//get all VIEW in the db at once o.O
				auto sqlInfo = F(R"(
SELECT TABLE_NAME,VIEW_DEFINITION,ALGORITHM 
FROM information_schema.VIEWS 
WHERE table_schema='{}')",
				                 dbName);
				auto res     = db->query(sqlInfo);
				for (auto& row : res) {
					auto tableName = row.rq("TABLE_NAME");
					schemas[{dbName, tableName}].push_back(row);
				}
			}
		}
	}

	return schemas;
}

void CheckSchema::saveSchema() {
	QSaveFile file("dbSchema");
	if (file.open(QFile::WriteOnly | QFile::Truncate)) {
		QByteArray  stream;
		QDataStream out(&stream, QIODevice::WriteOnly);
		out << getDbSchema();
		//auto sha = sha1(stream, false).toHex();
		//auto sz  = stream.size();
		file.write(stream);
	}
	file.commit();
}

CheckSchema::Schemas CheckSchema::loadSchema() {
	CheckSchema::Schemas map;
	QFile                file(":/db/dbSchema");
	if (file.open(QFile::ReadOnly)) {
		QByteArray payload;
		auto       content = file.readAll();
		//auto sha                = sha1(content, false).toHex();
		file.seek(0);
		QDataStream in(&file);
		in >> map;
		if (auto s = in.status(); s != QDataStream::Ok) {
			qCritical() << "error decoding stream: " << asString(s);
		}
	} else {
		qCritical() << "missing :/dbSchema file in the QRC !, create the symlink ecc ecc";
	}
	return map;
}
/*
if (conf().skipDbCheck) {
        return true;
}
*/
bool CheckSchema::checkDbSchema() {
	auto diskSchemas = loadSchema();
	auto dbSchemas   = getDbSchema();
	bool dirty       = false;
	for (auto&& [table, dbSchema] : dbSchemas) {
		auto diskSchema = diskSchemas.take(table);
		if (diskSchema == dbSchema) {
			continue; // exact same CREATE TABLE result, nice!
		}

		if (diskSchema.size() != dbSchema.size()) { // different number of lines
			auto msg      = F16("schema for {}.{} has different number of lines!\n", table.database, table.table);
			auto diskSize = diskSchema.size();
			auto dbSize   = dbSchema.size();
			auto size     = std::max(diskSize, dbSize);
			for (int i = 0; i < size; i++) {
				auto diskRow = diskSchema.value(i);
				auto dbRow   = dbSchema.value(i);
				auto diskCol = diskRow.value("COLUMN_NAME", "***NOTHING***");
				auto dbCol   = dbRow.value("COLUMN_NAME", "***NOTHING***");
				msg += F16("\t{} - {}\n", diskCol, dbCol);
			}
			qWarning().noquote() << msg;
			continue; // exact same CREATE TABLE result, nice!
		}
		// at this stage we are sure that the sizes of the 2 QStringList are the same
		for (int i = 0; i < diskSchema.size(); i++) {
			auto        diskLines = diskSchema[i];
			auto        dbLines   = dbSchema[i];
			QStringList diff;
			QByteArray  tableColumnName;
			diskLines.getIfNotNull("TABLE_NAME", tableColumnName);  //when processing view
			diskLines.getIfNotNull("COLUMN_NAME", tableColumnName); //when processing table

			for (const auto& [parameterName, diskValue] : diskLines) {
				auto dbValue = dbLines[parameterName];
				if (diskValue == dbValue) {
					continue;
				}

				// the two lines are definitely different
				diff.push_back(F16("------\nIn Table {}.{}::{} for {}:\nDisk:\n{}\nDB:\n{}", table.database, table.table, tableColumnName, parameterName, diskValue, dbValue));
			}

			if (!diff.empty()) {
				qWarning().noquote() << "schema for " << table << " is different!\n"
				                     << diff.join(", \n");
				dirty = true;
			}
		}
	}
	if (!diskSchemas.isEmpty()) {
		auto msg = F16("unknown table found:\n");

		for (auto&& [table, schema] : diskSchemas) {
			msg += F16("\t{}.{}\n", table.database, table.table);
		}
		qWarning().noquote() << msg;
		dirty = true;
	}
	if (dirty) {
		abort();
	}

	return true;
}

QDebug& operator<<(QDebug& out, const CheckSchema::Key& key) {
	QDebugStateSaver stateSaver(out);
	out.space().noquote() << key.database << key.table;
	return out;
}

CheckSchema::CheckSchema(DB* db_, QStringList database_)
    : db(db_), databases(database_) {
}

QDataStream& operator<<(QDataStream& out, const CheckSchema::Key& key) {
	out << key.database;
	out << key.table;
	return out;
}

QDataStream& operator>>(QDataStream& in, CheckSchema::Key& key) {
	in >> key.database;
	in >> key.table;
	return in;
}
