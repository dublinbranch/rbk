#ifndef LOCALEV2_H
#define LOCALEV2_H

#include <QString>
#include <rbk/SpaceShipOP/qstringship.h>
#define QSL(str) QStringLiteral(str)

class Locale {
      public:
	QString nation;
	QString language;
	// weak chekc when constructing the this obj (no exception if mkt and HAL both empty)
	bool weakCheck = false;

	Locale() = default;
	Locale(const QStringRef& nat, const QStringRef& lang);
	Locale(const QString& string, QString* ip = nullptr);
	bool    fromString(const QString& string, QString* ip = nullptr);
	bool    isNull();
	QString getString(const QString& style = QSL("xx-XX")) const;
	auto    operator<=>(const Locale& lhs) const = default;
};

/**
 * @brief decodeLocale
 * @param locale
 * @param if ip is set and valid, in case the locale is invalid we will try to use the IP to find the nation
 * @return
 */
Locale decodeLocale(const QString& locale, QString* ip = nullptr);

#endif // LOCALEV2_H
