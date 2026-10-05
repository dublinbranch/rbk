#include "QDebugHandler.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/filesystem/folder.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/gitTrick/buffer.h"
#include "rbk/misc/runnableV2.h"
//#include "slacksender.h"
//#include "twilio.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QLoggingCategory>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sys/stat.h>
#include <thread>

#ifdef useMinCurl
#include "rbk/minCurl/curlpp.h"
#endif

static const NanoSpammerConfig  configDefault;
static const NanoSpammerConfig* config = &configDefault;

static bool initLocaleTZDone = false;

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

void shutdownQtLogging() {
	qInstallMessageHandler(nullptr);
}

bool hasCurlSupport() {
#ifdef useMinCurl
	return true;
#else
	return false;
#endif
}

void requireCurlIfWarningMailEnabled(const NanoSpammerConfig& spamConf) {
	if (spamConf.warningToMail && !hasCurlSupport()) {
		throw ExceptionV2(
		    "warningToMail is enabled, but curl support is not compiled in "
		    "(build rbk with RBK_WITH_MINCURL / useMinCurl)!");
	}
}

QString getHeader1() {
	// header 1
	auto time           = QDateTime::currentDateTime().toString(Qt::ISODate);
	auto warningHeader1 = F16("@ {} From {} instanceId {} rev {}", time, QCoreApplication::applicationName(), config->instanceName, GIT_STATUS_buffer);
	return warningHeader1;
}

QString getHeader2(const char* file, int line, const char* func) {
	auto warningHeader2 = QSL("%1:%2 (%3)")
	                          .arg(file)
	                          .arg(line)
	                          .arg(func);
	return warningHeader2;
}

// void sendSlack(const QString& msg, std::string channel) {
// 	if (!config->slackOpt.warningON) {
// 		return;
// 	}
// 	if (channel.empty()) {
// 		channel = config->slackOpt.warningChannel;
// 	}

// 	SlackSender::sendAsync(channel, msg);
// }

// void callViaTwilio() {
// 	if (config->BRUTAL_INHUMAN_REPORTING) {
// 		std::thread twilio(Twilio::call);
// 		twilio.detach();
// 	}
// }

void sendMail(QString subject, QString message) {
	if (!config->warningToMail) {
		return;
	}

#ifdef useMinCurl
	for (auto& recipient : config->warningMailRecipients) {
		//NOTE this operation is "slow" so we need a detached thread
		auto CurlPPisBroken = [=]() {
			CURLpp marx = CURLpp::Builder()
			                  .set_email_details(message.toUtf8().constData(), subject.toUtf8().constData(), recipient.data())
			                  .set_smtp_details("spammer@seisho.us", "mjsydiTODNmDLTUqRIZY", "spammer@seisho.us", "smtp://seisho.us:25")
			                  .build();
			auto   res  = marx.perform();
			if (!res.has_value()) {
				std::cerr << res.error();
			}
		};
		std::thread Carlo(CurlPPisBroken);
		//The only real problem of this approach, is that if the program immediately exit, nothing will be sent
		//We can survive
		Carlo.detach();
	}
#else
	(void)subject;
	(void)message;
	throw ExceptionV2("asked to send a mail, but curl support is not compiled in!");
#endif
}

//In loving memory of 80 / 72 char punch card
static int lineLenght   = 80;
static int initialSpace = 5;

std::string submoduleInfo() {
	//is just easier to read the text output that try to use git api to have a clean message
	static const QVector<QByteArray> stopWords{"Entering", "Entrando"};
	std::string                      final;
	QFile                            submoduleInfo(":/submoduleInfo");
	if (!submoduleInfo.open(QFile::ReadOnly)) {
		return {};
	}

	QByteArray moduleName;
	while (true) {
		auto line = submoduleInfo.readLine();
		if (line.isEmpty()) {
			break;
		}
		bool dirty = false;
		for (auto& word : stopWords) {
			if (line.contains(word)) {
				line.replace(word, "");
				line.replace("'", "");
				moduleName = line.trimmed();
				dirty      = true;
				break;
			}
		}
		if (dirty) {
			continue;
		}
		auto padding = lineLenght - moduleName.size() - initialSpace - 41 - 2;
		auto f       = fmt::format(R"({3:{4}}{0}:{3:{2}}{1})", moduleName, line, padding, "", initialSpace + 2);
		final.append(f);
	}
	return final;
}

//This file is always recompiled to the macro for COMPILATION_TIME and GIT status con be updated continuously
//If you do not pile garbage is usully under a second the whole thing
/**
 * @brief commonInitialization
 * @param _config is a reference, as the config will be modified later probably
 * we set this initialization very early with some default value
 */
void commonInitialization(const NanoSpammerConfig* _config) {
	initLocaleTZ();

	// enable the printing
	QLoggingCategory::setFilterRules("*.debug=true");

	qInstallMessageHandler(generalMsgHandler);
	setlinebuf(stdout);
	setlinebuf(stderr);

	config = _config;

	std::string header;

	header.append("\x1B[31m"); //light red this is the bash color delimiter
	header += fmt::format(
	    R"(
{0:{3}}{0:*>{3}}{0:*>{2}}{0:*>{3}}
{0:{3}}{0:*>{3}}{1: ^{2}}{0:*>{3}}
{0:{3}}{0:*>{3}}{0:*>{2}}{0:*>{3}}

)",
	    "",
	    QCoreApplication::applicationName().toStdString(),
	    lineLenght - initialSpace * 3,
	    initialSpace);

	header += "\x1B[0m";    //end of bash color delimiter;
	header += "\x1B[0;32m"; //end of bash color delimiter;
	header += fmt::format("{0:{3}}GIT_STATUS:{0:{2}}{1}\n", "", GIT_STATUS_buffer, lineLenght - initialSpace - 11 - 40, initialSpace);
	header += fmt::format("{0:{3}}COMPILATION_TIME:{1: >{2}} UTC\n", "", COMPILATION_TIME_buffer, lineLenght - initialSpace - 21, initialSpace);
	header += fmt::format("{0:{2}}STARTED:{0:{3}}{1} UTC\n", "", config->startedAt.toString("yyyy-MM-dd HH:mm:ss"), initialSpace, lineLenght - initialSpace - 31);
	header += fmt::format("{0:{3}}PID:{1: >{2}}\n", "", QCoreApplication::applicationPid(), lineLenght - initialSpace - 4, initialSpace);
	header += "\n";
	header += fmt::format("{0:{1}}GIT_MODULES:\n", "", initialSpace);
	header += submoduleInfo();
	header += "\n";
	header += fmt::format("{0:{2}}{0:*^{1}}", "", lineLenght - initialSpace, initialSpace);
	header += "\x1B[0m";
	header += "\n\n";
	echo(header);
	/*

	           R"(
	GIT_STATUS:        {1: >{6}}
	COMPILATION_TIME:  {2: >{6}} UTC
	PID:               {3: >{6}}

	GIT_MODULES:
{4}
	                   )"
	           "\x1B[0m\n",
	           config->applicationName.toStdString(),
	           GIT_STATUS_buffer,
	           COMPILATION_TIME_buffer,
	           QCoreApplication::applicationPid(),
	           submoduleInfo(),
	                   lineLenght-10,
	                   lineLenght-10,
	                   "");
	                   */
}

namespace {
// Qt calls the message handler from many threads at once: one mutex guards the files and the
// prints. Built on first use, not at load: a QFile is a QObject. Leaked on purpose: must stay
// usable during static teardown.
struct LogState {
	std::timed_mutex m;
	QFile            logFile;
	QFile            errFile;
	// Warning mail rate limit, key from warningMailKey().
	rbk::RunnableV2 mailGate;
};

LogState& logState() {
	static LogState& s = *new LogState;
	return s;
}
} // namespace

bool tryWriteDiskLog(std::string_view line) {
	auto&            ls = logState();
	std::unique_lock lock(ls.m, std::try_to_lock);
	if (!lock.owns_lock() || !ls.errFile.isOpen()) {
		return false;
	}
	return ls.errFile.write(line.data(), static_cast<qint64>(line.size())) == static_cast<qint64>(line.size());
}

bool tryLogLine(bool error, std::string_view line, std::chrono::milliseconds wait) {
	auto&            ls = logState();
	std::unique_lock lock(ls.m, wait);
	if (!lock.owns_lock()) {
		return false;
	}
	auto  time = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
	auto  msg  = F("{} {}\n----------\n", time, line);
	auto& file = error ? ls.errFile : ls.logFile;
	if (file.isOpen()) {
		file.write(msg.data(), static_cast<qint64>(msg.size()));
	}
	std::FILE* stream = error ? stderr : stdout;
	fmt::print(stream, "{}", msg);
	fflush(stream);
	return true;
}

std::string warningMailKey(const QMessageLogContext& context, const QString& msg) {
	// file:line, not the text: the text contains URLs and times.
	if (context.file) {
		return F("{}:{}", context.file, context.line);
	}
	// No file (Qt's own release libraries, or code built without QT_MESSAGELOGCONTEXT): use the start
	// of the text, else all those messages share one key and block each other.
	// This is free text, against the RunnableV2 advice (bounded keys). Accepted: each new key is also a
	// mail, and before the cooldown each of these messages was a mail too.
	return msg.left(120).toStdString();
}

//QDebug send in stderr, but we want to use stdout
void generalMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
	// Qt static destructors run after QCoreApplication and can still emit QDebug.
	// Opening QFile (a QObject) in that window hits a destroyed vtable (pure virtual abort).
	if (!QCoreApplication::instance()) {
		std::FILE* stream = (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) ? stderr : stdout;
		fmt::print(stream, "{}\n", msg.toStdString());
		return;
	}

	// Held to the end: covers the lazy open, firstStdErrEvent, the disk write and the print.
	// A message logged from inside the handler cannot deadlock: Qt sends it to stderr instead.
	auto&           ls = logState();
	std::lock_guard lock(ls.m);
	auto&           logFile = ls.logFile;
	auto&           errFile = ls.errFile;

	// Unbuffered: every message reaches the kernel at once, so no exit path (graceful, forced,
	// SIGKILL, crash) loses the tail of the log.
	if (!logFile.isOpen()) {
		mkdir("log");
		auto time = QDateTime::currentDateTime().toString(mysqlDateTimeFormat);
		logFile.setFileName(QString("log/%1.log").arg(time));
		logFile.open(QIODevice::Append | QIODevice::Text | QIODevice::Unbuffered);

		errFile.setFileName(QString("log/%1.err").arg(time));
		errFile.open(QIODevice::Append | QIODevice::Text | QIODevice::Unbuffered);
	}

	//Qt 6.6 for *REASON* QsaveFile spam "Empty filename passed to function", but makes no sense
	static const QString why = "Empty filename passed to function";
	if (msg == why) {
		return;
	}
	if (msg.contains("The cached device pixel ratio value was stale on window")) {
		return;
	}

	//Used to send the current git revision just once, when we encounter an stderr level message, that is usually via mail
	static bool firstStdErrEvent = true;
	auto        time             = QDateTime::currentDateTime().toString(Qt::ISODate);

	QByteArray localMsg = msg.toUtf8();
	std::FILE* stream   = nullptr;
	auto       file     = context.file;

	if (context.file == nullptr) {
		file = "NOT VALID FILE ";
	} else {
		//remove the initial ../ just boring
		file = file + 3;
	}

	auto funkz = context.function;
	if (funkz == nullptr) {
		funkz = "NOT VALID FUNCTION";
	}

	QFile* diskLog = nullptr;

	switch (type) {
	case QtDebugMsg:
	case QtInfoMsg:
		stream  = stdout;
		diskLog = &logFile;
		break;
	case QtCriticalMsg:
		[[fallthrough]];
	case QtFatalMsg:
		//callViaTwilio();
		[[fallthrough]];
	case QtWarningMsg:
		if (firstStdErrEvent) {
			//This makes sense only in swapTronic and other program with one shot execution style, continuos execution one... not really...
			firstStdErrEvent = false;

			//If this macro is not found, add
			//DEFINES += GIT_CURRENT_SHA1='\\"$(shell git -C '$$_PRO_FILE_PWD_' rev-parse HEAD)\\"'
			//in the .pro file
			localMsg.prepend(QBL("Git revision: ") + GIT_STATUS_buffer + QBL("\n\n *************** \n \n"));
		}

		auto warningHeader1 = getHeader1();
		auto warningHeader2 = getHeader2(file, context.line, funkz);

		// {
		// 	QString msg2slack = QSL("<@U93PHQ62J> ") + warningHeader1 + QSL("\n") + warningHeader2 + QSL("\n\n") + msg;
		// 	sendSlack(msg2slack, config->slackOpt.warningChannel);
		// }
		if (config->warningToMail) {
			// Only the mail is limited: terminal and disk log still get every message.
			auto gate = ls.mailGate(warningMailKey(context, msg), config->mailCooldownSec.value_or(kDefaultMailCooldownSec));
			if (gate) {
				// subject
				auto subject = QSL("Error from %1 @ %2 in %3").arg(QCoreApplication::applicationName(), config->instanceName, funkz);
				// message
				auto warningMessage = warningHeader1 + QSL("<br/>") + warningHeader2 + QSL("<br/><br/><pre>") + msg + "</pre>";
				if (gate.suppressed > 0) {
					auto since = QDateTime::fromSecsSinceEpoch(gate.firstSuppressedSec).toString(Qt::ISODate);
					warningMessage += F16("<br/>{} similar messages suppressed since {}", gate.suppressed, since);
				}
				sendMail(subject, warningMessage);
			}
		}

		stream  = stderr;
		diskLog = &errFile;
		break;
	}

	auto msgFinal = F("{} {}:{} ({})\n{}\n----------\n",
	                  time,
	                  file,
	                  context.line,
	                  funkz,
	                  localMsg);

	if (diskLog) {
		diskLog->write(QByteArray::fromStdString(msgFinal));
	}

	fmt::print(stream, "{}", msgFinal);
	if (stream) {
		fflush(stream);
	}
}

//QDebug send in stderr, but we want to use stdout
void lowSpamMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
	Q_UNUSED(context);
	QByteArray localMsg = msg.toLocal8Bit();
	std::FILE* stream   = nullptr;
	switch (type) {
	case QtDebugMsg:
		stream = stdout;
		break;
	case QtInfoMsg:
		stream = stdout;
		break;
	case QtWarningMsg:
		stream = stderr;
		break;
	case QtCriticalMsg:
		stream = stderr;
		break;
	case QtFatalMsg:
		stream = stderr;
		break;
	}
	fprintf(stream, "%s\n", localMsg.constData());
}

void initLocaleTZ() {
	if (initLocaleTZDone) {
		return;
	}
	initLocaleTZDone = true;
	srand((uint)time(NULL));

#ifdef useMinCurl
	curl_global_init(CURL_GLOBAL_ALL);
#endif

	loadBuffer();

	//We are server side we do not care about human broken standard
	std::setlocale(LC_NUMERIC, "C");

	//If EaRTh iS FLAAAT why timezone ?!11!!?
#ifdef _WIN32
	// Windows: Use _putenv
	_putenv("TZ=UTC");
#else
	// POSIX: Use setenv
	setenv("TZ", "UTC", 1);
#endif

	tzset();
}

void callViaTwilio() {
}
