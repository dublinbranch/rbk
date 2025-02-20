#include "taginvoke.h"
#include "rbk/QStacker/exceptionv2.h"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <boost/json.hpp>
#include <fmt/core.h>
#include <list>
#include <qdatetime.h>

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t) {
	jv = t.toStdString();
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t) {
	jv = t.toStdString();
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QStringList& t) {
	jv = bj::value_from(std::list<QString>(t.begin(), t.end()));
}

QByteArray tag_invoke(const boost::json::value_to_tag<QByteArray>&, const boost::json::value& jv) {
	auto       s = jv.as_string();
	QByteArray q;
	q.setRawData(s.data(), static_cast<uint>(s.size()));
	q.detach();
	return q;
}

QString tag_invoke(const boost::json::value_to_tag<QString>&, const boost::json::value& jv) {
	auto& string = jv.as_string();
#if QT_VERSION_MAJOR == 5
	return QString::fromUtf8(string.data(), (int)string.size());
#elif QT_VERSION_MAJOR == 6
	return QString::fromUtf8(string.data(), string.size());
#endif
}

QTime tag_invoke(bj::value_to_tag<QTime>, bj::value const& v) {
	//return std::chrono::duration_cast<Duration>(v.at("seconds").as_double() * 1s);

	int h = 0, m = 0, s = 0;

	auto& a = v.as_array();
	switch (a.size()) {
	case 0:
		throw ExceptionV2("is this a joke ?");
	case 1:
		h = a[0].to_number<int>();
		[[fallthrough]];
	case 2:
		m = a[0].to_number<int>();
		[[fallthrough]];
	case 3:
		s = a[0].to_number<int>();
		break;
	default:
		throw ExceptionV2("what I am supposed to do ?");
		break;
	}
	return QTime{h, m, s};
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QTime& t) {
	//jv = {{"seconds", static_cast<double>(t / 1.0s)}};
	jv = bj::array({t.hour(), t.minute(), t.second()});
}

void boost::json::tag_invoke(const value_from_tag&, value& jv, const std::filesystem::__cxx11::path& t) {
	jv = t.string();
}

std::filesystem::__cxx11::path boost::json::tag_invoke(const bj::value_to_tag<std::filesystem::__cxx11::path>&, const value& jv) {
	std::filesystem::path path = std::string_view(jv.as_string());
	return path;
}
