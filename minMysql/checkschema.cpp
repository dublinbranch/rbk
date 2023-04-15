#include "checkschema.h"
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
			auto sqlTableList = QSL("select table_name from information_schema.tables where table_type='BASE TABLE' and table_schema='%1'").arg(dbName);
			auto res          = db->query(sqlTableList);
			for (auto&& line : res) {
				QString tbl       = line.value(QBL("table_name"));
				auto    sqlSchema = QSL("SHOW CREATE TABLE `%1`.`%2`").arg(dbName, tbl);
				auto    res2      = db->query(sqlSchema);
				QString createT   = res2.at(0).value(QBL("Create Table"));
				removeAutoInc(createT);
				schemas.insert({dbName, tbl}, createT);
			}
		}
		{
			auto sqlViewList = QSL("select table_name from information_schema.tables where table_type='VIEW' and table_schema='%1'").arg(dbName);
			auto res         = db->query(sqlViewList);
			for (auto&& line : res) {
				QString tbl       = line.value(QBL("table_name"));
				auto    sqlSchema = QSL("SHOW CREATE VIEW `%1`.`%2`").arg(dbName, tbl);
				auto    res2      = db->query(sqlSchema);
				schemas.insert({dbName, tbl}, res2.at(0).value(QBL("Create View")));
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

	for (auto&& [key, dbSchema] : dbSchemas) {
		auto diskSchema = diskSchemas.take(key);
		if (diskSchema == dbSchema) {
			continue; // exact same CREATE TABLE result, nice!
		}

		auto        sdlines = diskSchema.split("\n");
		auto        itlines = dbSchema.split("\n");
		QStringList diff;
		if (sdlines.size() != itlines.size()) { // different number of lines
			qWarning().noquote() << "schema for " << key << " has different number of lines!\n"
			                     << diskSchema << "\n-----------------------------\n"
			                     << dbSchema;
			abort();
		}

		// at this stage we are sure that the sizes of the 2 QStringList are the same
		for (int i = 0; i < sdlines.size(); ++i) {
			if (sdlines.at(i) == itlines.at(i)) {
				continue;
			}
			// the two raw lines are different.

			// different versions of InnoDB have different outputs regarding the DEFAULT NULL options
			QString sdline = sdlines.at(i);
			QString itline = itlines.at(i);
			sdline.replace("  ", " ");
			itline.replace("  ", " ");
			sdline.replace(" DEFAULT NULL", "");
			itline.replace(" DEFAULT NULL", "");
			if (sdline == itline) {
				continue;
			}

			// the definer of the views depends on the user that created the view
			sdline.replace(deleteDefiner, "");
			itline.replace(deleteDefiner, "");
			if (sdline == itline) {
				continue;
			}

			// the two lines are definitely different
			diff.push_back("------\n" + sdline + " \n" + itline);
		}

		if (!diff.empty()) {
			qWarning().noquote() << "schema for " << key << " is different!\n"
			                     << diff.join(", \n");
			abort();
		}
	}
	if (!diskSchemas.isEmpty()) {
		qWarning().noquote() << "unknown table found:\n"
		                     << diskSchemas;
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
