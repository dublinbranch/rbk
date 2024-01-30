#include "folder.h"
#include "filefunction.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <mutex>

bool mkdir(const QString& dirName) {
	static std::mutex            lock;
	std::scoped_lock<std::mutex> scoped(lock);
	QDir                         dir = QDir(dirName);
	if (!dir.mkpath(".")) {
		qWarning().noquote() << "impossible to create working dir" << dirName << "\n"
		                     << "maybe" << QCoreApplication::applicationName() << "is running without the necessary privileges";
		return false;
	}
	return true;
}
namespace RBK {
bool mkdir(const std::string& dirName) {
	return mkdir(QString::fromStdString(dirName));
}

bool mkdir(const char* dirName) {
	return mkdir(QString(dirName));
}
} // namespace RBK

void cleanFolder(const QString& folder) {
	auto dir = QDir(folder);
	dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
	auto files = dir.entryList();
	for (const auto& file : files) {
		dir.remove(file);
	}
}

QString getMostRecent(const QString pathDir, const QString& filter) {
	auto dir = QDir(pathDir);
	if (!filter.isEmpty()) {
		QStringList filters;
		filters << filter;
		dir.setNameFilters(filters);
	}

	dir.setSorting(QDir::Time);
	auto files = dir.entryList();
	if (!files.isEmpty()) {
		return pathDir + "/" + files.at(0);
	}
	return QString();
}
//something like /tmp/path/whatever.sql.*
QStringList search(const QString& path) {
	QFileInfo fInfo(path);
	auto      dir  = fInfo.absoluteDir();
	auto      list = dir.entryList({fInfo.fileName()}, QDir::Files);
	for (auto& row : list) {
		row = row.prepend(dir.path() + "/");
	}
	return list;
}

uint erase(const QStringList& files) {
	uint erased = 0;
	for (auto& file : files) {
		if (QFile(file).remove()) {
			erased++;
		}
	}
	return erased;
}

QString hardLinkFolder(const QString& source, const QString& dest, HLParam param) {
	mkdir(dest);
	for (auto& file : search(source)) {
		QFileInfo s(file);
		auto      d = dest + "/" + s.fileName();
		if (auto res = hardlink(file, d, param); !res.isEmpty()) {
			return res;
		}
	}
	return {};
}

bool rmdirV2(const QStringViewV2& path) {
	QDir dir(path);
	if (dir.exists()) {
		return dir.removeRecursively();
	}
	return false;
}

