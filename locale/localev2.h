#ifndef LOCALEV2_H
#define LOCALEV2_H

#include "rbk/SpaceShipOP/qstringship.h"
#include "rbk/mapExtensor/mapV2.h"
#include <QString>

//#define QSL(str) QStringLiteral(str)

class Locale {
      public:
	QString nation;

	//TODO Language need to became a class
	QString language;
	// weak chekc when constructing the this obj (no exception if mkt and HAL both empty)
	bool weakCheck = false;
	//small tweak for our logic, it means for this nations this is the main language
	bool main = false;

	Locale() = default;
	Locale(const QString& nat, const QString& lang);
	Locale(const QStringView& nat, const QStringView& lang);
	Locale(const QString& string, QString* ip = nullptr);
	bool    fromString(const QString& string, QString* ip = nullptr);
	bool    isNull();
	QString getString(const QString& style = QSL("xx-XX")) const;
	auto    operator<=>(const Locale& lhs) const = default;
	void    setLanguage(const QString& newLanguage);
};

/**
 * @brief decodeLocale
 * @param locale
 * @param if ip is set and valid, in case the locale is invalid we will try to use the IP to find the nation
 * @return
 */
Locale decodeLocale(const QString& locale, QString* ip = nullptr);

using Nation = QString;

class DB;
const mapV2<Nation, QVector<Locale>>& localeDB(DB* db, std::string sql = {});
const mapV2<Nation, QString>&         languageDB(DB* db, std::string sql = {});
#endif // LOCALEV2_H
