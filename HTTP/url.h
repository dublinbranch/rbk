#ifndef URL_H
#define URL_H

#include "rbk/QStacker/httpexception.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/fmtExtra/customformatter.h"
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

	QString join() const;

	//Todo maybe is better to mark if a parameter has been swapped than to remove it ?
	std::optional<std::string> checkIfUnused() const;
	void                       checkIfUnused(bool) const;
};

class Url {
      public:
	QUrl        url;   //usato per il path e alcune funzioni per la query
	QueryParams query; //più comodo per sapere cosa contiene la query
	QString     full;  //per comodità nel debug

	QString path;

	Url() = default;
	Url(const QStringAdt& _url, bool fix = true);

	void        set(const QStringAdt& _url, bool fix = true);
	std::string prettyPrint() const;
	QString     getHostNoWWW() const;

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
		QString print() const;
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
