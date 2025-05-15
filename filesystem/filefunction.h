#pragma once

#include "ffCommon.h"
#include "rbk/string/stringoso.h"
#include <QFile>

inline const QString FSDateTimeFormat             = "yyyy-MM-dd_HH:mm:ss";
inline const QString FSDateTimeMilliSecondsFormat = "yyyy-MM-dd_HH:mm:ss.zzz";

class QFileXT : public QFile {
      public:
	QFileXT() = default;
	QFileXT(const QString& file);
	bool open(QIODevice::OpenMode flags = QIODevice::OpenModeFlag::ReadOnly) override;
	bool open(QIODevice::OpenMode flags, bool quiet);
};

class QSaveV2 : public QFile {
      public:
	bool open(OpenMode flags) override;
	bool open(OpenMode flags, bool quiet);
};

struct FileGetRes {
	           operator bool();
	QByteArray content;
	bool       exist = false;
};

struct FPCRes {
	bool                   ok;
	QFileDevice::FileError error = QFileDevice::FileError::NoError;
	                       operator bool();
};

/**
 * given a path this will first check ON DISK if we have a local copy
 * if we have the local one, expecially used for develop/debug to avoid recompiled it will have the priority
 * else will use the embedded resource
 */
QString resourceTryDisk(const QString& fileName);

FPCRes filePutContents(const QByteAdt& pay, const QStringAdt& fileName, bool verbose = false);
bool   fileAppendContents(const QByteAdt& pay, const QStringAdt& fileName);

[[nodiscard]] QByteArray fileGetContents(const QString& fileName, bool quiet = true);
[[nodiscard]] QByteArray fileGetContents(const QString& fileName, bool quiet, bool& success);

[[nodiscard]] FileGetRes fileGetContents2(const QByteAdt& fileName, bool quiet = true, uint maxAge = 0);

[[nodiscard]] QByteArray unzip1(QByteArray zipped);

//TODO why are here ?
/**
  The parameter line MUST be kept alive, so the QStringRef can point to something valid
*/
//Much slower but more flexible, is that ever used ?
std::vector<QStringView> readCSVRowFlexySlow(const QString& line, const QStringList& separator = {","}, const QStringList& escape = {"\""});
//Quite fast expecially if optimizer is on

//how to init a QChar to the null char ?

std::vector<QStringView> readCSVRow(const QStringView& line, const QChar& separator = ',', const QChar& escape = '\0');
std::vector<QStringView> readCSVRow(const QString& line, const QChar& separator = ',', const QChar& escape = '\0');

std::vector<QByteArray> csvExploder(QByteArray line, const char separator = 0);

/**
 * @brief checkFileLock
 * @param path
 * @param minDelay, used to avoid to restart too soon (usefull in case of continuous crash restart loop), if 0 disable the delay
 * @param critical, use qCritical and will try to send a mail with the error
 */
void checkFileLock(QString path, uint minDelay = 5, bool critical = true);

bool softlink(const QString& source, const QString& dest, bool quiet = false);

//using namespace magic_enum::bitwise_operators;
// HLParam::quiet | HLParam::eraseOld
QString hardlink(const QString& source, const QString& dest, HLParam param = HLParam::eraseOld);

/**
 * @brief RotableFile is used to provide an in program logrotate functionality
 * @param name_
 * @param suffix
 * @return
 */
QString RotableFile(const QString& name, QString suffix = "");

void logWithTime(const QString& logFile, const QString& msg);
void logWithTime(const QString& logFile, const std::string& msg);
void logWithTime(const std::string& logFile, const std::string& msg);
void logWithTime(const QString& logFile, const QByteArray& msg);

/**
 * @brief innerOrDynamic will try to find a file in the dynamic path, if absent will load from
 * the internal path
 * @param innerPath
 * @param dynamicPath
 * @return
 */
struct FileResV2 {
	QString    path;
	QByteArray content;
	enum Type {
		Dynamic,
		Inner,
		missing
	} type;
	operator bool();
};

FileResV2 innerOrDynamic(const QString& innerPath, const QString& dynamicPath, bool quiet = true);

std::filesystem::path GetCurExecutablePath();

std::filesystem::path getTempFile();
QString               getTempFile(const QString&);

static const QString prettyTable = R"EOD(
<!DOCTYPE html><html role='document'><head><meta charset='utf-8'><meta content='en' name='language'>
<style>
 .chameleon-table {
font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
border-collapse: collapse;
width: 100%;
}

 .chameleon-table td, .chameleon-table th  {
border: 1px solid #ddd;
padding: 8px;
}

 .chameleon-table td:not(:first-child){
text-align:right;
}

 .chameleon-table tr:nth-child(even){background-color: #f2f2f2;}

 .chameleon-table tr:hover {background-color: #ddd;}

 .chameleon-table th , .chameleon-table th {
padding-top: 12px;
padding-bottom: 12px;z
text-align: left;
background-color: #4CAF50;
color: white;
}

</style>
</head>
<body>
<table class="chameleon-table">
<thead>
)EOD";
