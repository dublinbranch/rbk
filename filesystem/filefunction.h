#pragma once

#include "ffCommon.h"
#include <QFile>

inline const QString FSDateTimeFormat             = "yyyy-MM-dd_HH:mm:ss";
inline const QString FSDateTimeMilliSecondsFormat = "yyyy-MM-dd_HH:mm:ss.zzz";

class QFileXT : public QFile {
      public:
	QFileXT() = default;
	QFileXT(const QString& file);
	bool open(OpenMode flags) override;
	bool open(OpenMode flags, bool quiet);
};

class QSaveV2 : public QFile {
      public:
	bool open(OpenMode flags) override;
	bool open(OpenMode flags, bool quiet);
};

struct FileGetRes {
	operator bool() {
		return exist;
	}
	QByteArray content;
	bool       exist = false;
};

struct FPCRes {
	bool                   ok;
	QFileDevice::FileError error = QFileDevice::FileError::NoError;
	                       operator bool();
};

//I do really need to find a solution to this nonsense!
FPCRes filePutContents(const QString& pay, const QString& fileName);
FPCRes filePutContents(const QByteArray& pay, const QString& fileName);
FPCRes filePutContents(const std::string& pay, const QString& fileName);
FPCRes filePutContents(const std::string& pay, const std::string& fileName);
FPCRes filePutContents(const QByteArray& pay, const std::string& fileName);
FPCRes filePutContents(const QByteArray& pay, const char* fileName);

QByteArray fileGetContents(const QString& fileName, bool quiet = true);
QByteArray fileGetContents(const QString& fileName, bool quiet, bool& success);

FileGetRes fileGetContents2(const QString& fileName, bool quiet = true, uint maxAge = 0);
FileGetRes fileGetContents2(const std::string& fileName, bool quiet = true, uint maxAge = 0);

bool fileAppendContents(const QString& pay, const QString& fileName);
bool fileAppendContents(const QByteArray& pay, const QString& fileName);
bool fileAppendContents(const std::string& pay, const QString& fileName);
bool fileAppendContents(const std::string& pay, const std::string& fileName);

QByteArray unzip1(QByteArray zipped);

/**
  The parameter line MUST be kept alive, so the QStringRef can point to something valid
*/
//Much slower but more flexible, is that ever used ?
std::vector<QStringRef> readCSVRowFlexySlow(const QString& line, const QStringList& separator = {","}, const QStringList& escape = {"\""});
//Quite fast expecially if optimizer is on
std::vector<QStringRef> readCSVRow(const QStringRef& line, const QChar& separator = ',', const QChar& escape = 0x0);
std::vector<QStringRef> readCSVRow(const QString& line, const QChar& separator = ',', const QChar& escape = 0x0);

std::vector<QByteArray> csvExploder(QByteArray line, const char separator = 0);

void checkFileLock(QString path, uint minDelay = 5);

bool softlink(const QString& source, const QString& dest, bool quiet = false);

//using namespace magic_enum::bitwise_operators;
// HLParam::quiet | HLParam::eraseOld
QString hardlink(const QString& source, const QString& dest, HLParam param = HLParam::eraseOld);

void logWithTime(const QString& logFile, const QString& msg);
void logWithTime(const QString& logFile, const std::string& msg);
