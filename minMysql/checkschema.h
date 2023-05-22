#ifndef TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO
#define TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO

#include "rbk/minMysql/sqlresult.h"
#include "rbk/SpaceShipOP/qstringship.h"
#include "rbk/mapExtensor/qmapV2.h"
#include <QDebug>
#include <QStringList>

class DB;

class CheckSchema {
      public:
	struct Key {
		QString database;
		QString table;
		auto    operator<=>(const Key&) const = default;
	};

	using Schemas = QMapV2<Key, sqlResult>;

	explicit CheckSchema(DB* db_, QStringList database_);
	bool checkDbSchema();

	//This MUST be intentionally called when schema is updated
	//Remember to also add into the QRC file
	void saveSchema();

	Schemas getDbSchema();
	Schemas loadSchema();

      private:
	DB*         db = nullptr;
	QStringList databases;
};
QDebug&      operator<<(QDebug& d, const CheckSchema::Key& key);
QDataStream& operator<<(QDataStream& out, const CheckSchema::Key& key);
QDataStream& operator>>(QDataStream& in, const CheckSchema::Key& key);
#endif // TMP_QTCREATOR_HJDWZN_CLANGTOOLS_VFSO_OXXLMP_CHECKSCHEMA_H_AUTO
