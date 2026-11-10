#pragma once
#include "rbk/filesystem/filefunction.h"
#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <mutex>
#include <QDebug>

template <typename T>
qint64 fileSerialize(const QString& fileName, const T& t) {
	//Names that are in write now, or that wait for the write lock: a second call for one of these names does nothing.
	//namesLock is held only for the check, never during the disk work.
	static std::mutex    namesLock;
	static QSet<QString> busyNames;
	{
		std::lock_guard<std::mutex> scoped(namesLock);
		if (busyNames.contains(fileName)) {
			return 0;
		}
		busyNames.insert(fileName);
	}
	//Runs on every return path, also after an open or commit error, so the name cannot stay busy
	auto releaseName = qScopeGuard([&] {
		std::lock_guard<std::mutex> scoped(namesLock);
		busyNames.remove(fileName);
	});

	static std::mutex           lock;
	std::lock_guard<std::mutex> scoped(lock);

	QSaveFile file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::QIODevice::Truncate | QIODevice::WriteOnly)) {
		qCritical() << "Failed to open file for writing:" + fileName + " reason : " + file.errorString();
		return 0;
	}

	QDataStream out(&file);
	out.setVersion(QDataStream::Qt_5_15);
	out << t;

	if (!file.commit()) {
		qCritical() << "Failed to write data to file:" + fileName + " reason: " + file.errorString();
		return false;
	}

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

	if (in.status() != QDataStream::Ok) {
		qCritical() << "Failed to deserialize data from file:" + fileName;
		return {0, true, lastEdit, false};
	}
	return {file.size(), true, lastEdit, true};
}
