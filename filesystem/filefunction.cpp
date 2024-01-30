#include "filefunction.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII/resetAfterUse.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/fmtExtra/customformatter.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/magicEnum/magic_enum.hpp"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/serialization/serialize.h"
#include "rbk/string/qstringview.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSaveFile>
#include <boost/tokenizer.hpp>
#include <mutex>
#include <sys/file.h>
#include <zip.h>

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

QFileXT::QFileXT(const QString& file) {
	setFileName(file);
}

bool QFileXT::open(QIODevice::OpenMode flags) {
	return open(flags, false);
}

bool QFileXT::open(QIODevice::OpenMode flags, bool quiet) {
	if (!QFile::open(flags)) {
		if (!quiet) {
			qCritical().noquote() << errorString() << "opening" << fileName() + QStacker16Light();
		}
		return false;
	}
	return true;
}

bool QSaveV2::open(QIODevice::OpenMode flags) {
	return open(flags, false);
}

bool QSaveV2::open(QIODevice::OpenMode flags, bool quiet) {
	if (!QFile::open(flags)) {
		if (!quiet) {
			qWarning().noquote() << errorString() << "opening" << fileName() + QStacker16Light();
		}
		return false;
	}
	return true;
}

FPCRes filePutContents(const QByteArray& pay, const QString& fileName, bool verbose) {
	QSaveFile file;
	file.setFileName(fileName);
	// TODO nel caso il file non sia scribile (di un altro utente) ritorna un vaghissimo WriteError, indicare se possibile meglio!
	if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
		if (verbose) {
			qCritical().noquote() << F16("Impossbile to write into {} due to {} in {}\n", fileName, asSWString(file.error()), QStacker16Light());
		}
		return {false, file.error()};
	}
	auto written = file.write(pay);
	if (written != pay.size()) {
		if (verbose) {
			qCritical().noquote() << F16("Impossbile to write into {} due to {} in {}\n", fileName, asSWString(file.error()), QStacker16Light());
		}
		return {false, file.error()};
	}
	file.commit();
	return {true};
}

QByteArray fileGetContents(const QString& fileName, bool quiet) {
	bool ok;
	auto x = fileGetContents(fileName, quiet, ok);
	return x;
}

FileGetRes fileGetContents2(const QByteViewV2& fileName, bool quiet, uint maxAge) {
	FileGetRes res;
	if (maxAge) {
		auto cTime = QFileInfo(fileName).lastModified().toSecsSinceEpoch();
		auto age   = QDateTime::currentSecsSinceEpoch() - cTime;
		if (age > maxAge) {
			return res;
		}
	}

	res.content = fileGetContents(fileName, quiet, res.exist);

	return res;
}

QByteArray fileGetContents(const QString& fileName, bool quiet, bool& success) {
	if (fileName.isEmpty()) {
		return QByteArray();
	}
	QFileXT file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::ReadOnly, quiet)) {
		return QByteArray();
	}
	success = true;
	auto x  = file.readAll();
	return x;
}

bool fileAppendContents(const QByteArray& pay, const QString& fileName) {
	static std::mutex           mutex;
	std::lock_guard<std::mutex> lock(mutex);
	QFileXT                     file;
	file.setFileName(fileName);
	if (!file.open(QIODevice::Append | QIODevice::WriteOnly)) {
		//TODO
		// fare gestione dell'errore come in filePutContents
		qWarning().noquote() << "not possible write data in file" << fileName << QStacker16Light();

		return false;
	}
	auto byteWritten = file.write(pay);
	if (byteWritten != pay.size()) {
		qCritical() << F16("Error writing into {}, wrote {} byte, should have been {}\n", fileName, byteWritten, pay.size());
	}
	file.flush();
	file.write("\n");
	file.close();
	return true;
}

// returns the (unzipped) content of a zipped file.
// zipped = content of the zipped file
QByteArray unzip1(QByteArray zipped) {
	zip_error_t error;
	zip_error_init(&error);
	auto            src = zip_source_buffer_create(zipped, (zip_uint64_t)zipped.size(), 1, &error);
	auto            za  = zip_open_from_source(src, 0, &error);
	struct zip_stat sb;
	for (auto i = 0; i < zip_get_num_entries(za, 0); i++) {
		if (zip_stat_index(za, i, 0, &sb) == 0) {
			//			printf("==================\n");
			//			auto len = strlen(sb.name);
			//			printf("Name: %s\n, ", sb.name);
			//			printf("Size: %lu\n, ", sb.size);
			//			printf("mtime: %u\n", (unsigned int)sb.mtime);
			//			fflush( stdout );
			auto zf = zip_fopen_index(za, i, 0);
			if (!zf) {
				qCritical().noquote() << "error iterating zip file" << QStacker16();
				return QByteArray();
			}

			QByteArray decompressed;
			decompressed.resize(static_cast<int>(sb.size));
			auto len = zip_fread(zf, decompressed.data(), sb.size);
			if (len < 0) {
				qCritical().noquote() << "error decompressing zip file" << QStacker16();
				return QByteArray();
			}
			zip_fclose(zf);
			// nothing to free as the lib is buggy and tries to deallocate the
			// original buffer -.-
			return decompressed;
		}
	}
	// some error
	qDebug().noquote() << "something strange with that zip file" << QStacker16();
	return QByteArray();
}

std::vector<QByteArray> csvExploder(QByteArray line, const char separator) {
	// The csv we receive is trash sometimes
	line.replace(QBL("\r"), QByteArray());
	line.replace(QBL("\n"), QByteArray());
	// Not only we can receive a new line symbol, we can receive the two character \r or \n o.O
	line.replace(QBL("\\r"), QByteArray());
	line.replace(QBL("\\n"), QByteArray());

	// https://www.boost.org/doc/libs/1_71_0/libs/tokenizer/doc/char_separator.htm
	using Tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;
	boost::escaped_list_separator<char> sep('\\', ',', '\"');
	if (separator) {
		sep = boost::escaped_list_separator<char>('\\', separator, '\"');
	}
	std::vector<QByteArray>  final;
	std::vector<std::string> vec2;
	auto                     cry = line.toStdString();
	try {
		ResetOnExit r(cxaLevel, CxaLevel::none);
		Tokenizer   tok(cry, sep);
		vec2.assign(tok.begin(), tok.end());
	} catch (...) {
		// qWarning().noquote() << "error decoding csv line " << line;
		return final;
	}
	// Cry
	for (auto&& l : vec2) {
		final.push_back(QByteArray::fromStdString(l));
	}
	return final;
}

using namespace std::literals;
void checkFileLock(QString path, uint minDelay) {
	// check if there is another instance running...

	int fd = open(path.toUtf8().data(), O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
		qWarning() << path << "error opening" << path;
		exit(1);
	}

	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		auto msg = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss ") + path + " is already locked, I refuse to start.\n (The application is already running.) ";
		qDebug().noquote() << msg;
		sleep(minDelay);
		exit(1);
	}

	auto pathTs = path + ".lastExec";
	if (minDelay) {
		QByteArray x;
		auto       f = fileUnSerialize(pathTs, x, minDelay);
		if (f.fileExists && f.valid) {
			auto msg = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss ") + QSL(" file %1 is NOT locked, but is too recent, last application start was less than %2 second ago (so we will wait a bit to avoid spamming)").arg(path).arg(minDelay);
			qWarning() << msg;
			sleep(minDelay);
			exit(1);
		}
	}
	filePutContents(pathTs, pathTs);
}

FPCRes filePutContents(const QString& pay, const QString& fileName, bool verbose) {
	return filePutContents(pay.toUtf8(), fileName, verbose);
}

FPCRes filePutContents(const std::string& pay, const QString& fileName, bool verbose) {
	return filePutContents(QByteArray::fromStdString(pay), fileName, verbose);
}

FPCRes filePutContents(const std::string& pay, const std::string& fileName, bool verbose) {
	return filePutContents(QByteArray::fromStdString(pay), QString::fromStdString(fileName), verbose);
}

FPCRes filePutContents(const QByteArray& pay, const std::string& fileName, bool verbose) {
	return filePutContents(pay, QString::fromStdString(fileName), verbose);
}

FPCRes filePutContents(const QByteArray& pay, const char* fileName, bool verbose) {
	return filePutContents(pay, QString(fileName), verbose);
}

bool fileAppendContents(const std::string& pay, const QString& fileName) {
	return fileAppendContents(QByteArray::fromStdString(pay), fileName);
}

// Much slower but more flexible, is that ever used ?
std::vector<QStringView> readCSVRowFlexySlow(const QString& line, const QStringList& separator, const QStringList& escape) {
	std::vector<QStringView> part;
	if (line.isEmpty()) {
		return part;
	}
	part.reserve(10);
	// Quando si ESCE dal quote, non avanzare di pos, altrimenti diventa "ciao" -> ciao"
	// questo innesca il problema che se vi è ad esempio ciao,"miao""bau",altro
	// invece di avere ciao, miaobau, altro non funge perché il range NON é CONTIGUO -.-, ma viene ritornato ad esempio ciao, miao, bau, altro

	// newState = delta[currentState][event]

	// rows are states

	// columns are events:
	// 0 = read separator
	// 1 = read escape
	// 2 = read new line
	// 3 = read "normal" character
	// 4 = reached the end of line (eof if line is a file content)
	static const int delta[][5] = {
	    //  ,    "   \n    ?  eof
	    {1, 2, -1, 0, -1}, // 0: parsing (store char)
	    {1, 2, -1, 0, -1}, // 1: parsing (store column)
	    {3, 4, 3, 3, -2},  // 2: quote entered
	    {3, 4, 3, 3, -2},  // 3: parsing inside quotes (store char)
	    {1, 3, -1, 0, -1}, // 4: quote exited
	                       // -1: end of row, store column, success
	                       // -2: eof inside quotes
	};

	static const QChar   newline = '\n';
	static const QString empty;

	// initial value
	int actualState = 0;
	int pos         = 0;

	int event             = -1;
	int currentBlockStart = -1;
	int blockEnd          = 0;

	auto le = line.length();
	while (actualState >= 0) {
		if (pos >= le)
			event = 4;
		else {
			auto ch = QStringView(line.data() + pos, 1);
			pos++;
			if (separator.contains(ch, Qt::CaseSensitive))
				event = 0;
			else if (escape.contains(ch, Qt::CaseSensitive)) {
				event = 1;
			} else if (ch == newline)
				event = 2;
			else
				event = 3;
		}

		actualState = delta[actualState][event];

		switch (actualState) {
		case 4:
			blockEnd = pos - 1;
			break;
		case 2:
			currentBlockStart = pos;
			break;
		case 0:
		case 3:
			if (currentBlockStart == -1) {
				currentBlockStart = pos - 1;
			}

			break;
		case -1:
		case 1:

			if (!blockEnd) {
				blockEnd = pos - 1;
			}
			if (currentBlockStart == -1) { // a new block has never started, we have two separator in a row
				part.push_back(empty);
				blockEnd = 0;
			} else {
				auto v   = midView(line, currentBlockStart, (blockEnd - currentBlockStart));
				blockEnd = 0;
				part.push_back(v);
				currentBlockStart = -1;
				// curentBlock.clear();
			}
			break;
		}
	}
	if (actualState == -2) {
		throw ExceptionV2("Line terminated while inside quotes.");
	}

	return part;
}

std::vector<QStringView> readCSVRow(const QString& line, const QChar& separator, const QChar& escape) {
	return readCSVRow(QStringView(line), separator, escape);
}

std::vector<QStringView> readCSVRow(const QStringView& line, const QChar& separator, const QChar& escape) {
	std::vector<QStringView> part;
	if (line.isEmpty()) {
		return part;
	}
	part.reserve(1024);
	// Quando si ESCE dal quote, non avanzare di pos, altrimenti diventa "ciao" -> ciao"
	// questo innesca il problema che se vi è ad esempio ciao,"miao""bau",altro
	// invece di avere ciao, miaobau, altro non funge perché il range NON é CONTIGUO -.-, ma viene ritornato ad esempio ciao, miao, bau, altro

	// newState = delta[currentState][event]

	// rows are states

	// columns are events:
	// 0 = read separator
	// 1 = read escape
	// 2 = read new line
	// 3 = read "normal" character
	// 4 = reached the end of line (eof if line is a file content)
	static const int delta[][5] = {
	    //  ,    "   \n    ?  eof
	    {1, 2, -1, 0, -1}, // 0: parsing (store char)
	    {1, 2, -1, 0, -1}, // 1: parsing (store column)
	    {3, 4, 3, 3, -2},  // 2: quote entered (no-op)
	    {3, 4, 3, 3, -2},  // 3: parsing inside quotes (store char)
	    {1, 3, -1, 0, -1}, // 4: quote exited (no-op)
	                       // -1: end of row, store column, success
	                       // -2: eof inside quotes
	};

	static const QChar   newline = '\n';
	static const QString empty;

	// initial value
	int actualState = 0;
	int pos         = 0;

	int event             = -1;
	int currentBlockStart = -1;
	int blockEnd          = 0;

	QChar ch;

	auto le = line.length();
	while (actualState >= 0) {
		if (pos >= le) {
			event = 4;
		} else {
			ch = line[pos];
			if (ch == separator) {
				event = 0;
			} else if (ch == escape) {
				event = 1;
			} else if (ch == newline) {
				event = 2;
			} else
				event = 3;
		}
		pos++;

		actualState = delta[actualState][event];

		switch (actualState) {
		case 4:
			blockEnd = pos - 1;
			break;
		case 0:
		case 3:
			if (currentBlockStart == -1) {
				currentBlockStart = pos - 1;
			}

			break;
		case -1:
		case 1:
			if (!blockEnd) {
				blockEnd = pos - 1;
			}
			if (currentBlockStart == -1) { // a new block has never started, we have two separator in a row
				part.push_back(empty);
				blockEnd = 0;
			} else {
				auto numbOfChars = blockEnd - currentBlockStart;
				part.push_back(line.mid(currentBlockStart, numbOfChars));
				blockEnd          = 0;
				currentBlockStart = -1;
			}
			break;
		}
	}
	if (actualState == -2) {
		throw ExceptionV2("Line terminated while inside quotes.");
	}

	return part;
}

bool softlink(const QString& source, const QString& dest, bool quiet) {
	auto res = QFile(source).link(dest);
	if (!res && !quiet) {
		qWarning() << QSL("error soft symlinking %1 to %2").arg(source, dest);
	}
	return res;
}

QString hardlink(const QString& source, const QString& dest, HLParam param) {
	using namespace magic_enum::bitwise_operators;
	QString msg;
	if ((param & HLParam::eraseOld) == HLParam::eraseOld) {
		QFile(dest).remove();
	}
	auto fail = link(source.toUtf8().constData(), dest.toUtf8().constData());
	if (fail == -1) {
		msg.append(strerror(errno));

		if ((param & HLParam::quiet) == HLParam::quiet) {
			qCritical() << QSL("error hard symlinking %1 to %2, msg is %3").arg(source, dest, msg);
		}
	}
	return msg;
}

FPCRes::operator bool() {
	return ok;
}

void logWithTime(const std::string& logFile, const std::string& msg) {
	auto now    = QDateTime::currentDateTimeUtc().toString(mysqlDateTimeFormat).toStdString();
	auto logMsg = "\n-------------------\n" + now + " UTC\n" + msg + "\n";

	fileAppendContents(logMsg, logFile);
}

void logWithTime(const QString& logFile, const QString& msg) {
	auto now    = QDateTime::currentDateTimeUtc().toString(mysqlDateTimeFormat);
	auto logMsg = QSL("\n-------------------\n %1 UTC\n%2\n")
	                  .arg(now, msg);
	fileAppendContents(logMsg.toUtf8(), logFile);
}

void logWithTime(const QString& logFile, const std::string& msg) {
	auto now    = QDateTime::currentDateTimeUtc().toString(mysqlDateTimeFormat).toStdString();
	auto logMsg = "\n-------------------\n" + now + "UTC\n" + msg + "\n";

	fileAppendContents(logMsg, logFile);
}

void logWithTime(const QString& logFile, const QByteArray& msg) {
	logWithTime(logFile, msg.toStdString());
}

bool fileAppendContents(const QString& pay, const QString& fileName) {
	return fileAppendContents(pay.toUtf8(), fileName);
}

bool fileAppendContents(const std::string& pay, const std::string& fileName) {
	return fileAppendContents(QByteArray::fromStdString(pay), QString::fromStdString(fileName));
}

QString RotableFile(const QString& name_, QString suffix) {
	auto now = QDate::currentDate();
	//2010-01-03
	auto n2 = now.toString(Qt::ISODate);
	if (!suffix.isEmpty()) {
		suffix.prepend(".");
	}
	return name_ + "_" + n2 + suffix;
}

QString resourceTryDisk(const QString& fileName) {
	if (QFile::exists(fileName)) {
		return fileName;
	}
	//if you play weird trick with path, you are looking for troubles
	return ":/" + fileName;
}

FileResV2 innerOrDynamic(const QString& innerPath, const QString& dynamicPath) {
	FileResV2 final;
	{
		auto res = fileGetContents2(dynamicPath, true, 0);
		if (res.exist) {
			final.path    = dynamicPath;
			final.content = res.content;
			final.type    = FileResV2::Dynamic;
			return final;
		}
	}

	{
		auto res = fileGetContents2(innerPath, false, 0);
		if (res.exist) {
			final.path    = innerPath;
			final.content = res.content;
			final.type    = FileResV2::Inner;
			return final;
		}
	}
	final.type = FileResV2::missing;
	return final;
}

FileGetRes::operator bool() {
	return exist;
}

FileResV2::operator bool() {
	return type != Type::missing;
}

std::filesystem::path GetCurExecutablePath() {
	char                   result[PATH_MAX];
	ssize_t                count = readlink("/proc/self/exe", result, PATH_MAX);
	std::string            path  = std::string(result, (count > 0) ? count : 0);
	std::string::size_type pos   = path.find_last_of('/');
	return path.substr(0, pos);
}
