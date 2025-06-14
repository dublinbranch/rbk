#include "localev2.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/minMysql/min_mysql.h"

#ifdef WithMaxMind
#include "rbk/GeoLite2PP/GeoLite2PP.hpp"
extern GeoLite2PP::DB* mmdb;
#endif

using namespace std;
Locale::Locale(const QString& string, QString* ip) {
	*this = decodeLocale(string, ip);
}

void Locale::setLanguage(const QString& newLanguage) {
	if (newLanguage == "all") {
		language = "en";
		return;
	}
	language = newLanguage;
}

Locale::Locale(const QString& nat, const QString& lang) {
	nation   = nat.toUpper();
	language = lang.toLower();
}

Locale::Locale(const QStringView& nat, const QStringView& lang) {
	nation   = nat.toString().toUpper();
	language = lang.toString().toLower();
}

bool Locale::fromString(const QString& string, QString* ip) {
	*this = decodeLocale(string, ip);
	return true;
}

QString Locale::getString(const QString& style) const {

	if (style == "xx-XX") {
		return QSL("%1-%2")
		    .arg(this->language)
		    .arg(this->nation);
	}
	if (style == "xx_XX") {
		return QSL("%1_%2")
		    .arg(this->language)
		    .arg(this->nation);
	}
	if (style == "xx-xx") {
		return QSL("%1-%2")
		    .arg(this->language)
		    .arg(this->nation.toLower());
	}
	if (style == "xx_xx") {
		return QSL("%1_%2")
		    .arg(this->language)
		    .arg(this->nation.toLower());
	}

	throw ExceptionV2(QSL("Invalid style \"%1\" for locale").arg(style));
}

QList<QStringView> splitM1(QStringView o) {
	//hopefully they no longer add more separator
	if (o.contains('-')) {
		return o.split('-');
	} else if (o.contains('_')) {
		return o.split('_');
	} else {
		return {};
	}
}

Locale from2letter(const QStringView& locale, QString* ip) {
	if (!ip->isEmpty()) {
#ifdef WithMaxMind
		Locale res;
		res.language = locale.toString().toLower();
		auto geoMap  = mmdb->get_all_fields(ip->toStdString());
		res.nation   = QString::fromStdString(geoMap["country_iso_code"]).toUpper();
		return res;
#else
		throw ExceptionV2("MMDB not compiled in, use the flag WithMaxMind and provide one");
#endif
	}
	throw HttpException("Unable to decode locale " + locale.toString() + " please allow to use the ip to put a nation");
}

Locale decodeLocale(const QString& locale, QString* ip) {
	if (locale.size() == 2) { //just en
		return from2letter(locale, ip);
	}
	//WTF ??? TODO in teoria prendere se più avanti ci sono altri locale
	if (locale == QSL("en-XA")) {
		return Locale("en-US");
	}

	// this is the HTTP LANGUAGE PART which is like fr-DZ,fr;q=0.9,ar-DZ;q=0.8,ar;q=0.7,fr-CA;q=0.6,en-US;q=0.5,en;q=0.4

	//The problem is that many browser and device have no clue what they are doing so they can send
	//en,en-US;q=0.9
	//it,en;q=0.5
	//And other atrocities

	QString block1 = locale;
	if (locale.indexOf(";") > 0) {
		auto part = locale.split(';');
		block1    = part[0];
	}

	auto block1Part = block1.split(",");

	auto sz = block1Part.size();

	if (sz == 0) {
		throw HttpException("Unable to decode locale >" + locale + "<");
	}

	//case 0 fr-DZ,fr;q=0.9 (correct) block1Part = fr-DZ
	if (auto p2 = splitM1(block1Part[0]); p2.size() == 2) {
		return {p2[1], p2[0]};
	} else if (sz > 1) {

		{
			auto t1_p2 = block1Part[1];

			//for cases like de,*
			if (t1_p2 == QSL("*")) {
				if (auto t1_p1 = block1Part[0]; t1_p1.size() == 2) {
					return from2letter(t1_p1, ip);
				}
			}

			// de,en;q=0.9,en-GB;
			if (t1_p2.size() == 2) {
				if (auto t1_p1 = block1Part[0]; t1_p1.size() == 2) {
					return from2letter(t1_p1, ip);
				}
			}
		}

		if (auto p3 = splitM1(block1Part[1]); p3.size() == 2) {
			//case 1 en,en-US;q=0.9 (still good news)
			return {p3[1], p3[0]};
		}
	} else if (block1Part[0].size() == 2) { //case 2 it,en;q=0.5 (bad news)
		//Certain case like it_IT is fine, other like en_EN are wrong... we will retrieve for now the nation from the IP
		// cxaLevel == none -> automatic print of the exception messages is disabled
		return from2letter(block1Part[0], ip);
	}

	throw HttpException("Unable to decode locale >" + locale + "<");
}

bool Locale::isNull() {
	return nation.isEmpty() and language.isEmpty();
}

const mapV2<Nation, QVector<Locale>>& localeDB(DB* db, string sql) {
	static mapV2<Nation, QVector<Locale>> final;
	if (final.empty()) {
		//No need for lock control, just init at beginning of program and is fine
		if (sql.empty()) {
			sql = "SELECT nation, language, isMain FROM SCCE_Common.locale ORDER BY language ASC";
		}

		auto res = db->query(sql);
		for (auto& row : res) {
			auto   nat  = row.rq<QString>("nation");
			auto   lang = row.rq<QString>("language");
			Locale l(nat, lang);
			row.rq("isMain", l.main);

			final[l.nation].push_back(std::move(l));
		}
	}
	return final;
}

const mapV2<Nation, QString>& languageDB(DB* db, string sql) {
	static mapV2<Nation, QString> final;
	if (final.empty()) {
		//No need for lock control, just init ad beginning of program and is fine
		if (sql.empty()) {
			sql = "SELECT code, name FROM SCCE_Common.language ORDER BY code ASC";
		}

		auto res = db->query(sql);
		for (auto& row : res) {
			auto code   = row.rq<QString>("code");
			auto name   = row.rq<QString>("name");
			final[code] = name;
		}
	}
	return final;
}
