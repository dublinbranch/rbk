#pragma once
#include "rbk/filesystem/filefunction.h"
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QSaveFile>
#include <mutex>

template <typename T>
qint64 fileSerialize(const QString& fileName, const T& t) {
	static QString currentFile;

	//crude and imperfect way to avoid writing a file twice at the same time
	//Because putting a single lock will de facto make this single threaded even if we have all different files
	if (fileName == currentFile) {
		return 0;
	}

	static std::mutex           lock;
	std::lock_guard<std::mutex> scoped(lock);

	currentFile = fileName;

	QSaveFile file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::QIODevice::Truncate | QIODevice::WriteOnly)) {
		qCritical() << "Failed to open file for writing:" + file.errorString();
		return 0;
	}

	QDataStream out(&file);
	out.setVersion(QDataStream::Qt_5_15);
	out << t;

	if (!file.commit()) {
		qCritical() << "Failed to write data to file:" + file.errorString();
		return false;
	}

	currentFile.clear();
	return file.size();
}

struct UnserializeResult {
	qint64    size       = 0;
	bool      fileExists = false;
	QDateTime age;
	bool      valid = false;
};
template <typename T>
UnserializeResult fileUnSerialize(const QString& fileName, T& t, uint maxAge = 0) {
	if (!maxAge) {
		return {0, true, QDateTime::currentDateTime(), false};
	}
	QFileXT file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::ReadOnly, true)) {
		return {0, false, QDateTime()};
	}

	//AFAIK birthTime is a bit broken in linux http://moiseevigor.github.io/software/2015/01/30/get-file-creation-time-on-linux-with-ext4/
	auto lastEdit = QFileInfo(file).metadataChangeTime();
	//is the file fresh enought ?
	if (lastEdit.toSecsSinceEpoch() < QDateTime::currentSecsSinceEpoch() - maxAge) {
		//no is old!
		return {0, true, lastEdit, false};
	}

	QDataStream in(&file);
	in.setVersion(QDataStream::Qt_5_15);

	in >> t;
	return {file.size(), true, lastEdit, true};
}
