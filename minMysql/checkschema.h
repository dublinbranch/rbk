#ifndef TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO
#define TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO

#include "rbk/SpaceShipOP/qstringship.h"
#include "rbk/mapExtensor/qmapV2.h"
#include "rbk/minMysql/sqlresult.h"
#include <QDebug>
#include <QStringList>

class DB;

class CheckSchema {
      public:
	using ReMap = mapV2<QByteArray, sqlRow>;

	struct Key {
		QString database;
		QString table;
		auto    operator<=>(const Key&) const = default;
	};

	struct TableData {
		QString    sql;
		QString    name;
		QByteArray primaryKey;
	};

	using TableDatas = std::vector<TableData>;
	using Schemas    = QMapV2<Key, sqlResult>;

	explicit CheckSchema(DB* db_, QStringList database_);
	bool         checkDbSchema();
	bool         checkTableData(const TableDatas& td);
	//The key is trimmed, use checkWhitespace to warn about keys with whitespace
	static ReMap reMap(const sqlResult& raw, const QByteArray& pk);

	//This MUST be intentionally called when schema is updated
	//Remember to also add into the QRC file
	void saveSchema();

	//All or nothing: returns false and saves no file if a table has a bad key.
	//Call it before saveSchema, so the schema and the data are refreshed together or not at all.
	bool saveTableData(const TableDatas& td);

	Schemas    getDbSchema();
	Schemas    loadSchema();
	QByteArray loadSchemaInner();
	void       setBasePath(const QString& neu);

      private:
	DB*         db = nullptr;
	QStringList databases;

	//Where the rows come from: it decides what checkWhitespace scans and which fix SQL it prints
	enum class Origin {
		MachineDb, //the DB of this machine in checkTableData: keys only, the reference decides the other columns
		Disk,      //the reference data: keys and other columns, no fix SQL
		RefreshDb, //the source DB in saveTableData: keys and other columns, with fix SQL
	};

	//Returns false for a key with whitespace at the start or end, and if two keys are the same after trim
	//(reMap then keeps only the last of them).
	//Other columns with space or tab at the end: only a warning.
	bool checkWhitespace(const sqlResult& raw, const TableData& table, Origin origin) const;
};
QDebug&      operator<<(QDebug& d, const CheckSchema::Key& key);
QDataStream& operator<<(QDataStream& out, const CheckSchema::Key& key);
QDataStream& operator>>(QDataStream& in, const CheckSchema::Key& key);

void                     updateQRCFile();
void                     generateQrcFile(const QString& qrcFilePath, const std::vector<std::string>& filePaths, const std::string& resourcePrefix);
std::vector<std::string> getFilesInDirectory(const std::string& directoryPath);

#endif // TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO
