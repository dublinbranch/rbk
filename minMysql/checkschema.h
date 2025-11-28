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
	static ReMap reMap(const sqlResult& raw, const QByteArray& pk);

	//This MUST be intentionally called when schema is updated
	//Remember to also add into the QRC file
	void saveSchema();

	void saveTableData(const TableDatas& td);

	Schemas    getDbSchema();
	Schemas    loadSchema();
	QByteArray loadSchemaInner();

      private:
	DB*         db = nullptr;
	QStringList databases;
};
QDebug&      operator<<(QDebug& d, const CheckSchema::Key& key);
QDataStream& operator<<(QDataStream& out, const CheckSchema::Key& key);
QDataStream& operator>>(QDataStream& in, const CheckSchema::Key& key);

void                     updateQRCFile();
void                     generateQrcFile(const QString& qrcFilePath, const std::vector<std::string>& filePaths, const std::string& resourcePrefix);
std::vector<std::string> getFilesInDirectory(const std::string& directoryPath);

void CKSOverrideBasePath(const QString& neu);
#endif // TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO
