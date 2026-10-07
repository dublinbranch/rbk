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
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

//This is a macro passed during compilation, so we can save the schema in the right place
//So when the program run it will save the data INSIDE THE SOURCE folder
//TODO the problem is than when it run somewhere else... bad news
//So I have to pass this dynamically or in some other way when I am on the develop machine
//Or maybe just simlink there.... so data remain in the program folder
static QString basePath = BasePath;

//The QRC somethimes is bugged and, well just do by hand
//used to be CKSOverrideBasePath
void CheckSchema::setBasePath(const QString& neu) {
	basePath = neu;
}

void removeAutoInc(QString& sql) {
	static QRegularExpression regex(R"RX(AUTO_INCREMENT=(\d*))RX");
	sql.replace(regex, QString());
}

CheckSchema::Schemas CheckSchema::getDbSchema() {
	CheckSchema::Schemas schemas;

	QVector<QByteArray> views;

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
				//get all VIEW in the db at once o.O
				auto sqlInfo = F(R"(
SELECT TABLE_NAME,VIEW_DEFINITION,ALGORITHM
FROM information_schema.VIEWS
WHERE table_schema='{}')",
				                 dbName);
				auto res     = db->query(sqlInfo);
				for (auto& row : res) {
					auto tableName = row.rq("TABLE_NAME");
					views.append(F8("{}.{}", dbName, tableName));
				}
			}

			{
				//	,PRIVILEGES https://github.com/dublinbranch/rbk/issues/7
				//get all tables in the db at once o.O
				//but skip the view, as in some cases they might have different structure or defaults
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
					auto fullName  = F8("{}.{}", dbName, tableName);

					if (views.contains(fullName)) {
						row.insert("isView", "1");
					} else {
						row.insert("isView", "0");
					}

					schemas[{dbName, tableName}].push_back(row);
				}
			}
		}
	}

	return schemas;
}

void CheckSchema::saveSchema() {
	if (!mkdir(basePath + "/db")) {
		throw ExceptionV2("impossible creare la cartella!" + basePath + "/db");
	}
	auto      path = basePath + QSL("/db/schema");
	QSaveFile file(path);
	echo("Saving schema in {} ", path);
	if (file.open(QFile::WriteOnly | QFile::Truncate)) {
		QByteArray  stream;
		QDataStream out(&stream, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_15);
		out << getDbSchema();
		//auto sha = sha1(stream, false).toHex();
		//auto sz  = stream.size();
		file.write(stream);
		file.commit();
	} else {
		throw ExceptionV2(F("impossible to save schema: {} in {}", file.errorString(), path));
	}
}

//With a schema difference the key column can be missing, and rq would throw. All the rows of one result have the same columns.
static bool hasKeyColumn(const sqlResult& raw, const CheckSchema::TableData& table, const char* source) {
	if (raw.isEmpty() || raw.first().contains(table.primaryKey)) {
		return true;
	}
	echo("table {} : the key column {} is not in the {} data", table.name, table.primaryKey, source);
	return false;
}

bool CheckSchema::saveTableData(const TableDatas& td) {
	//All or nothing: a bad key in the reference data goes to every machine, and a partial save gives a new schema with old data.
	//A space or tab at the end of another column is only a warning.
	std::vector<sqlResult> results;
	bool                   ok = true;
	for (auto& row : td) {
		auto res = db->query(row.sql);
		if (!hasKeyColumn(res, row, "DB") || !checkWhitespace(res, row, Origin::RefreshDb)) {
			ok = false;
		}
		results.push_back(res);
	}
	if (!ok) {
		echo("A table has a key problem (see above), NO table data is saved and the old files are kept. Fix the DB and refresh again");
		return false;
	}

	mkdir(basePath + "/db");
	for (size_t i = 0; i < td.size(); i++) {
		auto      path = basePath + QSL("/db/") + td[i].name;
		QSaveFile file(path);
		echo("Saving table info in {} ", path);
		if (file.open(QFile::WriteOnly | QFile::Truncate)) {
			file.seek(0);
			QByteArray  stream;
			QDataStream out(&stream, QIODevice::WriteOnly);
			out.setVersion(QDataStream::Qt_5_15);

			out << results[i];

			file.write(stream);
		} else {
			qCritical() << file.errorString();
		}
		file.commit();
	}
	return true;
}

QByteArray CheckSchema::loadSchemaInner() {
	auto file = basePath + "/db/schema";
	auto res  = fileGetContents2(file, true, 0);
	if (res.exist) {
		echo("Db Schema loaded from file ({})", file);
		return res.content;
	}

	// __has_include is not reliable for #embed; __has_embed matches #embed's resource lookup.
#if __has_embed("db/schema") != __STDC_EMBED_NOT_FOUND__
	echo("Db Schema loaded from embedded");
	static const unsigned char data[] = {
#embed "db/schema"
	};
	QByteArray schema(
	    reinterpret_cast<const char*>(data),
	    static_cast<qsizetype>(sizeof(data)));
	return schema;
#else
#pragma message("missing db/schema, remember to run with --refreshDBSchema and recompile")
#endif
	return {};
}

CheckSchema::Schemas CheckSchema::loadSchema() {
	CheckSchema::Schemas map;

	auto schema = loadSchemaInner();
	if (schema.isEmpty()) {
		qCritical() << "no valid db/schema found, upload the file to perform the check!";
		abort();
	}

	QDataStream in(schema);
	in.setVersion(QDataStream::Qt_5_15);
	in >> map;
	if (auto s = in.status(); s != QDataStream::Ok) {
		string extraInfo;
		auto   msg = F16(R"(
error decoding stream:
dim:	{}
sha512:	{}
content:
{} 
	)",
		                 schema.size(), sha512(schema), schema);

		qCritical().noquote() << msg;
		abort();
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
			dirty = true;

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

			auto isView = dbLines.get2<u8>("isView");

			for (const auto& [parameterName, diskValue] : diskLines) {
				if (isView) {
					static const QVector<QByteArray> skipMe = {"COLUMN_DEFAULT", "IS_NULLABLE"};
					//certain column are unreliable between different db!
					if (skipMe.contains(parameterName)) {
						continue;
					}
				}
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

	//The caller decides what to do, so it can also run checkTableData before it stops
	return !dirty;
}

CheckSchema::ReMap CheckSchema::reMap(const sqlResult& raw, const QByteArray& pk) {
	ReMap reMap;
	for (auto& row : raw) {
		reMap[row.rq(pk).trimmed()] = row;
	}
	return reMap;
}

//Space or tab at the end of a value. Newline is out for now: in long text (pages, info) it can be correct
static auto trailingBlanks(const QByteArray& v) {
	decltype(v.size()) n = 0; //int in Qt5, qsizetype in Qt6
	//Generic placeholder, the spaces are on purpose
	static const QByteArrayList placeholders = {" - "};
	if (placeholders.contains(v)) {
		return n;
	}
	while (n < v.size()) {
		auto c = v.at(v.size() - 1 - n);
		if (c != ' ' && c != '\t') {
			break;
		}
		n++;
	}
	return n;
}

bool CheckSchema::checkWhitespace(const sqlResult& raw, const TableData& table, Origin origin) const {
	//A key with whitespace at the start or end is a data error, so it fails the check.
	//MariaDB (PAD SPACE) ignores trailing spaces in =, so SQL still finds the row, but C++ code that compares
	//the key bytes (== or a map) does not, and the value is lost without a word.
	auto                 source = origin == Origin::Disk ? "Disk" : "DB";
	std::set<QByteArray> seen;
	bool                 ok = true;
	for (auto& row : raw) {
		auto key     = row.rq(table.primaryKey);
		auto trimmed = key.trimmed();
		if (trimmed != key) {
			auto msg = F(R"(
table {} : {} = "{}" in {} has whitespace at the start or end ({} char, hex {}), the check uses "{}"
)",
			             table.name, table.primaryKey, key, source, key.size(), key.toHex(), trimmed);
			if (origin != Origin::Disk) {
				msg += F(R"(To update this should be ok
UPDATE {} SET {} = "{}" WHERE {} = "{}";
)",
				         table.name,
				         db->escape(table.primaryKey), db->escape(trimmed),
				         db->escape(table.primaryKey), db->escape(key));
			}
			echo(msg);
			ok = false;
		}
		if (!seen.insert(trimmed).second) {
			echo("table {} : {} = \"{}\" in {} is in more than one row after trim, only the last one is checked",
			     table.name, table.primaryKey, trimmed, source);
			ok = false;
		}

		//The other columns: a space or tab at the end is bad but not critical, so only warn.
		//On a machine the reference decides: a bad DB value shows as a data mismatch, with the fix to the reference value.
		//A fix SQL here would break the check when the reference has the same bad value.
		if (origin == Origin::MachineDb) {
			continue;
		}
		for (auto [col, value] : std::as_const(row)) {
			if (col == table.primaryKey) {
				continue;
			}
			auto n = trailingBlanks(value);
			if (n == 0) {
				continue;
			}
			auto msg = F(R"(
table {} : {} at row {} = "{}" in {} ends with space or tab ({} char, last {} hex {})
)",
			             table.name, col, table.primaryKey, trimmed, source, value.size(), n, value.right(n).toHex());
			//A long value (a page body) in the UPDATE fills the log at every start
			if (origin == Origin::RefreshDb && value.size() > 255) {
				msg += "The value is too long to print the UPDATE, fix it by hand\n";
			} else if (origin == Origin::RefreshDb) {
				msg += F(R"(To update this should be ok
UPDATE {} SET {} = "{}" WHERE {} = "{}";
)",
				         table.name,
				         db->escape(col), db->escape(value.chopped(n)),
				         db->escape(table.primaryKey), db->escape(key));
			}
			echo(msg);
		}
	}
	return ok;
}

bool CheckSchema::checkTableData(const TableDatas& td) {
	bool ok = true;
	for (auto& table : td) {
		auto inner   = QSL(":/db/") + table.name;
		auto dynamic = basePath + "/db/" + table.name;
		auto file    = innerOrDynamic(inner, dynamic, false);
		if (file.type == FileResV2::missing) {
			//Not fatal: the table data has no embedded copy (as db/schema has), so a binary built
			//somewhere else does not find it, and it must still start
			qCritical() << F16("impossible to load table data, tryed {} and {}, the data check for this table is skipped", inner, dynamic);
			continue;
		}
		echo("Table {} loaded from {}", table.name, file.path);
		QDataStream in(file.content);
		in.setVersion(QDataStream::Qt_5_15); //same as saveTableData

		sqlResult diskRawData;
		in >> diskRawData;
		if (auto s = in.status(); s != QDataStream::Ok) {
			echo("table {} : the reference data in {} is corrupt (QDataStream status {}), the data check for this table is skipped",
			     table.name, file.path, (int)s);
			ok = false;
			continue;
		}

		sqlResult dbRawData;
		try {
			dbRawData = db->query(table.sql);
		} catch (const std::exception& e) {
			//Most probably the schema is different (a column in the SELECT is missing), checkDbSchema printed the diff
			echo("table {} : the query failed, the data check for this table is skipped\n{}", table.name, e.what());
			ok = false;
			continue;
		}

		if (!hasKeyColumn(dbRawData, table, "DB") || !hasKeyColumn(diskRawData, table, "Disk")) {
			echo("table {} : the data check for this table is skipped", table.name);
			ok = false;
			continue;
		}

		if (!checkWhitespace(dbRawData, table, Origin::MachineDb)) {
			ok = false;
		}
		if (!checkWhitespace(diskRawData, table, Origin::Disk)) {
			ok = false;
		}

		auto dbData   = reMap(dbRawData, table.primaryKey);
		auto diskData = reMap(diskRawData, table.primaryKey);

		//The reference, the target is of COURSE the data on disk!

		//When the schema is different a column can be missing in the DB, report it once per table
		std::set<QByteArray> missingCols;

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
			//The key as it is in the DB, so the fix SQL also matches a key with whitespace
			auto dbKey = dbRow.rq(table.primaryKey);
			//now check the column if matches
			for (auto [kDisk, vDisk] : std::as_const(diskRow)) {
				//The rows are matched on the trimmed key, checkWhitespace already failed the check for the whitespace
				if (kDisk == table.primaryKey) {
					continue;
				}
				auto f = dbRow.fetch(kDisk);
				if (!f) {
					missingCols.insert(kDisk);
					ok = false;
					continue;
				}
				auto& vRow = *f.value;
				if (vRow != vDisk) {
					//Do not copy a bad reference value into the DB, checkWhitespace (Disk) already warned about it
					auto fix = trailingBlanks(vDisk)
					               ? F("The Disk value ends with space or tab, so the reference data is wrong.\n"
					                   "Fix the source DB and refresh, do not copy it into this DB")
					               : F(R"(To update this should be ok

UPDATE {}
SET {} = "{}"
WHERE {} = "{}"
;
)",
					                   table.name,
					                   db->escape(kDisk),
					                   db->escape(vDisk),
					                   db->escape(table.primaryKey),
					                   db->escape(dbKey));

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
		for (auto& col : missingCols) {
			echo("table {} column {} is in the reference data but not in the DB", table.name, col);
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
