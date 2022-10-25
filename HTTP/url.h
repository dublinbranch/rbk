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

	/**
	 * @brief setQuery
	 * @param val
	 */
	void setQuery(const QString& val);
	template <class T>
	bool swap(const QString& key, T& t) const {
		auto found = mapV2::get(key);
		if (found.found) {

			if constexpr (std::is_integral_v<T>) {
				bool ok = false;
				t       = found.val->toLongLong(&ok);
				if (ok) {
					return true;
				} else {
					throw HttpException(QSL("Impossible to convert %1 to int (was %2)").arg(key, *found.val));
				}
			} else if constexpr (std::is_floating_point_v<T>) {
				bool ok = false;
				t       = found.val->toDouble(&ok);
				if (ok) {
					return true;
				} else {
					throw HttpException(QSL("Impossible to convert %1 to float (was %2)").arg(key, *found.val));
				}
			} else {
				t = *found.val;
				return true;
			}
		}
		return false;
	}

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

	template <class T>
	bool swap(const QStringList& keys, T& t) const {
		for (auto& key : keys) {
			if (swap(key, t)) {
				return true;
			}
		}
		return false;
	}

	template <class T>
	void swapRq(const QStringList& keys, T& t) const {
		for (auto& key : keys) {
			if (swap(key, t)) {
				return;
			}
		}
		throw HttpException(QSL("Required parameter %1 is missing").arg(keys.join(" or ")));
	}

	template <class T>
	void swapRq(const QString& key, T& t) const {
		if (swap(key, t)) {
			return;
		}
		throw HttpException(QSL("Required parameter %1 is missing").arg(key));
	}

	template <class T>
	T swapRq(const QString& key) const {
		T t;
		swapRq(key, t);
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
