/**
 * To use this system read 
 * https://github.com/dublinbranch/rbk/wiki/Check-SChema
 */ 

#include "checkschema.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/hash/sha.h"
#include "rbk/minMysql/min_mysql.h"
#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

//This is a macro passed during compilation, so we can save the schema in the right place
//So when the program run it will save the data INSIDE THE SOURCE folder
//TODO the problem is than when it run somewhere else... bad news
//So I have to pass this dynamically or in some other way when I am on the develop machine
//Or maybe just simlink there.... so data remain in the program folder
const QString basePath = BasePath;

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
				//	,PRIVILEGES https://github.com/dublinbranch/rbk/issues/7
				//get all tables in the db at once o.O
				auto sqlInfo = F(R"(
SELECT
	TABLE_CATALOG
	,TABLE_SCHEMA
	,TABLE_NAME
	,COLUMN_NAME
	,ORDINAL_POSITION
	,COLUMN_DEFAULT
	,IS_NULLABLE
	,DATA_TYPE
	,CHARACTER_MAXIMUM_LENGTH
	,CHARACTER_OCTET_LENGTH
	,NUMERIC_PRECISION
	,NUMERIC_SCALE
	,DATETIME_PRECISION
	,CHARACTER_SET_NAME
	,COLLATION_NAME
	,COLUMN_TYPE
	,COLUMN_KEY
	,EXTRA
	,COLUMN_COMMENT
	,IS_GENERATED
	,GENERATION_EXPRESSION
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
	mkdir(basePath + "/db");
	auto      path = basePath + QSL("/db/schema");
	QSaveFile file(path);
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

void CheckSchema::saveTableData(const TableDatas& td) {
	mkdir(basePath + "/db");
	for (auto& row : td) {
		auto      path = basePath + QSL("/db/") + row.name;
		QSaveFile file(path);
		if (file.open(QFile::WriteOnly | QFile::Truncate)) {
			file.seek(0);
			QByteArray  stream;
			QDataStream out(&stream, QIODevice::WriteOnly);

			auto res = db->query(row.sql);
			out << res;

			file.write(stream);
		} else {
			qCritical() << file.errorString();
		}
		file.commit();
	}
}

CheckSchema::Schemas CheckSchema::loadSchema() {
	CheckSchema::Schemas map;
	auto                 inner(":/db/schema");
	auto                 dynamic = basePath + "/db/schema";
	auto                 file    = innerOrDynamic(inner, dynamic, false);
	if (file.type == FileResV2::missing) {
		qCritical() << F16("impossible to load schema, tryed {} and {}", inner, dynamic);
	}

	echo("DB schema loaded from {}", file.path);
	QDataStream in(file.content);
	in >> map;
	if (auto s = in.status(); s != QDataStream::Ok) {
		qCritical() << "error decoding stream: " << asString(s);
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
			auto msg = F16("schema for {}.{} has different number of lines!\n", table.database, table.table);

			auto rowCount = std::max(diskSchema.size(), dbSchema.size());
			struct X1 {
				QByteArray disk;
				QByteArray db;
			};

			vector<X1> buffer;

			qsizetype diskMaxSize = 0;

			for (int i = 0; i < rowCount; i++) {
				auto diskRow = diskSchema.value(i);
				auto dbRow   = dbSchema.value(i);
				auto diskCol = diskRow.value("COLUMN_NAME", "***NOTHING***");
				auto dbCol   = dbRow.value("COLUMN_NAME", "***NOTHING***");
				buffer.push_back({diskCol, dbCol});
				diskMaxSize = max(diskMaxSize, (qsizetype)diskCol.size());
			}

			msg += F16("\t{:>{}} - {}\n", "Disk", diskMaxSize, "DB");
			for (auto& [disk, dbVal] : buffer) {
				msg += F16("\t{:>{}} - {}\n", disk, diskMaxSize, dbVal);
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

CheckSchema::ReMap CheckSchema::reMap(const sqlResult& raw, const QByteArray& pk) {
	ReMap reMap;
	for (auto& row : raw) {
		reMap[row.rq(pk)] = row;
	}
	return reMap;
}

bool CheckSchema::checkTableData(const TableDatas& td) {
	bool ok = true;
	for (auto& table : td) {
		auto inner   = QSL(":/db/") + table.name;
		auto dynamic = basePath + QSL("/db/") + table.name;
		auto file    = innerOrDynamic(inner, dynamic, false);
		if (file.type == FileResV2::missing) {
			qCritical() << F16("impossible to load schema, tryed {} and {}", inner, dynamic);
		}
		echo("Table {} loaded from {}", table.name, file.path);
		QDataStream in(file.content);

		auto      dbRawData = db->query(table.sql);
		sqlResult diskRawData;
		in >> diskRawData;

		auto dbData   = reMap(dbRawData, table.primaryKey);
		auto diskData = reMap(diskRawData, table.primaryKey);

		//The reference, the target is of COURSE the data on disk!

		for (auto& [pk, diskRow] : diskData) {
			auto v = dbData.get(pk);
			//check if the dbRow even exists
			if (!v) {
				auto msg = F(R"(
table {} impossible to find the row {} = {}
				)",
				             table.name, table.primaryKey, pk);
				echo(msg);
				ok = false;
				continue;
			}
			auto& dbRow = *v.val;
			//now check the column if matches
			for (auto [kDisk, vDisk] : diskRow) {
				auto vRow = dbRow.rq(kDisk);
				if (vRow != vDisk) {
					auto fix = F(R"(
UPDATE {}
SET {} = "{}"
WHERE {} = "{}"
;
)",
					             table.name, db->escape(kDisk), db->escape(vDisk), db->escape(table.primaryKey), db->escape(pk));

					auto msg = F16(
					    R"(

table {} : {} data mismatch at row {} = {} :
Disk is ({} char):
---***---
{}
---***---
DB is ({} char):
---***---
{}
---***---
To update this should be ok
{}

---***------***------***------***---
)",
					    table.name, kDisk, table.primaryKey, pk,
					    vDisk.size(), vDisk,
					    vRow.size(), vRow,
					    fix);
					echo(msg);
					ok = false;
				}
			}
		}
	}
	return ok;
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

std::vector<std::string> getFilesInDirectory(const std::string& directoryPath) {
	std::vector<std::string> filePaths;

	for (const auto& entry : fs::directory_iterator(directoryPath)) {
		if (fs::is_regular_file(entry.path())) {
			filePaths.push_back(entry.path().string());
		}
	}

	return filePaths;
}

void generateQrcFile(const QString& qrcFilePath, const std::vector<std::string>& filePaths, const std::string& resourcePrefix) {
	QFileXT file(qrcFilePath);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate, false)) {
		exit(1);
	}
	string buffer = F(R"(
<RCC>
	<qresource prefix="{}">
)",
	                  resourcePrefix);
	//Qt will refuse a qrc with no files inside -.-, so we just add this soft check that will add itself in this case
	if (filePaths.empty()) {
		buffer += "	<file>../db.qrc</file>";
	} else {
		for (const auto& filePath : filePaths) {
			std::string relativePath = fs::relative(filePath, basePath.toStdString()).string();
			buffer += F("	<file>{}</file>\n", relativePath);
		}
	}

	buffer += "	</qresource>\n";
	buffer += "</RCC>\n";

	file.write(buffer.c_str(), (qint64)buffer.size());
}

void updateQRCFile() {
	auto qrcFilePath = basePath + "/db.qrc";
	auto filePaths   = getFilesInDirectory(basePath.toStdString() + "/db");
	generateQrcFile(qrcFilePath, filePaths, "");
}
