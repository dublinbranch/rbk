#include "checkschema.h"
#include "fmtExtra/dynamic.h"
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
				auto sqlTables = F("SELECT * FROM `TABLES` WHERE `TABLE_SCHEMA` = '{}'", dbName);
				auto res       = db->query(sqlTables);
				for (auto& row : res) {
					auto        tableName = row.rq("TABLE_NAME");
					auto        type      = row.rq("TABLE_TYPE");
					std::string sqlInfo;
					if (type != "BASE TABLE" || type != "VIEW") {
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
SELECT VIEW_DEFINITION,ALGORITHM 
FROM information_schema.VIEWS 
WHERE table_schema='{}'
ORDER BY `ORDINAL_POSITION` ASC)",
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
	static QRegularExpression deleteDefiner("DEFINER=(.*) SQL SECURITY DEFINER VIEW");

	auto diskSchemas = loadSchema();
	auto dbSchemas   = getDbSchema();
	bool dirty       = false;
	for (auto&& [key, dbSchema] : dbSchemas) {
		auto diskSchema = diskSchemas.take(key);
		if (diskSchema == dbSchema) {
			continue; // exact same CREATE TABLE result, nice!
		}

		auto        diskLines = diskSchema.split("\n");
		auto        dbLines   = dbSchema.split("\n");
		QStringList diff;
		if (diskLines.size() != dbLines.size()) { // different number of lines
			qWarning().noquote() << "schema for " << key << " has different number of lines!\n"
			                     << diskSchema << "\n-----------------------------\n"
			                     << dbSchema;
			abort();
		}

		// at this stage we are sure that the sizes of the 2 QStringList are the same
		for (int i = 0; i < diskLines.size(); ++i) {
			if (diskLines.at(i) == dbLines.at(i)) {
				continue;
			}
			// the two raw lines are different.

			// different versions of InnoDB have different outputs regarding the DEFAULT NULL options
			QString diskLine = diskLines.at(i);
			QString dbLine   = dbLines.at(i);
			diskLine.replace("  ", " ");
			dbLine.replace("  ", " ");
			diskLine.replace(" DEFAULT NULL", "");
			dbLine.replace(" DEFAULT NULL", "");
			if (diskLine == dbLine) {
				continue;
			}

			// the definer of the views depends on the user that created the view
			diskLine.replace(deleteDefiner, "");
			dbLine.replace(deleteDefiner, "");
			if (diskLine == dbLine) {
				continue;
			}

			// the two lines are definitely different
			diff.push_back("------\nDisk:\n" + diskLine + " \nDB:\n" + dbLine);
		}

		if (!diff.empty()) {
			qWarning().noquote() << "schema for " << key << " is different!\n"
			                     << diff.join(", \n");
			dirty = true;
		}
	}
	if (!diskSchemas.isEmpty()) {
		qWarning().noquote() << "unknown table found:\n"
		                     << diskSchemas;
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
