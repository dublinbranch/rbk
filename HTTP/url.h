#ifndef URL_H
#define URL_H

#include "rbk/QStacker/httpexception.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/mapExtensor/mapV2.h"
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

class QueryParams : public mapV2<QString, QString> {
      public:
	//Una inutile scocciatura che QueryParams non fa vedere il suo contenuto!
	//Todo proponi un Pretty Printer per QTCreator ?
	/**
	 * @brief setQuery
	 * @param val
	 */
	void setQuery(const QString& val);

	QString get64(const QString& key) const;
	bool    get64(const QString& key, QString& value) const;

	template <class T>
	T get2(const QString& key) const {
		T t = T();
		if (swap(key, t)) {
			return t;
		}
		return t;
	}

	QString join() const;
};

class Url {
      public:
	QUrl        url;   //usato per il path e alcune funzioni per la query
	QueryParams query; //più comodo per sapere cosa contiene la query
	QString     full;  //per comodità nel debug

	Url() = default;
	Url(const std::string& _url, bool fix = true);
	Url(const QString& _url, bool fix = true);
	Url(const QByteArray& _url, bool fix = true);
	void        set(const std::string& _url, bool fix = true);
	void        set(const QString& _url, bool fix = true);
	std::string prettyPrint() const;

	/**
	 * @brief get3lvl
	 * @return the 3rd level subdomain so in case of www.miao.it will be www
	 */
	QString get3lvl() const;
	/**
	 * @brief get2lvl
	 * @return the 2rd level subdomain so in case of www.miao.it will be miao
	 */
	QString get2lvl() const;

	QString getNlvl(int pos) const;

	struct DS {
		QString domain;
		QString subDomain;
	};

	DS      getDomain_SubDomain();
	QString getDomain();
	QString getSubDomain();

      private:
	DS ds;
};

QString fixBrokenUrl(QString broken);

#endif // URL_H
