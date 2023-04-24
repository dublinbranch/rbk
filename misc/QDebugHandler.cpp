#include "QDebugHandler.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/gitTrick/buffer.h"
#include "rbk/minCurl/curlpp.h"
#include "slacksender.h"
#include "twilio.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <curl/curl.h>
#include <thread>

static const NanoSpammerConfig* config = nullptr;

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

QString getHeader1() {
	// header 1
	auto time           = QDateTime::currentDateTime().toString(Qt::ISODate);
	auto warningHeader1 = QSL("@ %1 From %2 instanceId %3 rev %4")
	                          .arg(time)
	                          .arg(QCoreApplication::applicationName(), config->instanceName, GIT_STATUS_buffer);
	return warningHeader1;
}

QString getHeader2(const char* file, int line, const char* func) {
	auto warningHeader2 = QSL("%1:%2 (%3)")
	                          .arg(file)
	                          .arg(line)
	                          .arg(func);
	return warningHeader2;
}

void sendSlack(const QString& msg, std::string channel) {
	if (!config->slackOpt.warningON) {
		return;
	}
	if (channel.empty()) {
		channel = config->slackOpt.warningChannel;
	}

	SlackSender::sendAsync(channel, msg);
}
void callViaTwilio() {
	if (config->BRUTAL_INHUMAN_REPORTING) {
		std::thread twilio(Twilio::call);
		twilio.detach();
	}
}
void sendMail(QString subject, QString message) {
	if (!config->warningToMail) {
		return;
	}

	//	CURLpp marx = CURLpp::Builder()
	//	                  .set_email_details("ciao", "soggetto", "admin@seisho.us")
	//	                  .set_smtp_details("spammer@seisho.us", "mjsydiTODNmDLTUqRIZY", "spammer@seisho.us")
	//	                  .build();
	//	marx.perform();

	for (auto& recipient : config->warningMailRecipients) {
		//NOTE this operation is "slow" so we need a detached thread
		auto CurlPPisBroken = [=]() {
			CURLpp marx = CURLpp::Builder()
			                  .set_email_details(message.toUtf8().constData(), subject.toUtf8().constData(), recipient.constData())
			                  .set_smtp_details("spammer@seisho.us", "mjsydiTODNmDLTUqRIZY", "spammer@seisho.us")
			                  .build();
			marx.perform();
		};
		std::thread Carlo(CurlPPisBroken);
		//The only real problem of this approach, is that if the program immediately exit, nothing will be sent
		//We can survive
		Carlo.detach();
	}
}

//In loving memory of 80 / 72 char punch card
static uint lineLenght   = 80;
static uint initialSpace = 5;

std::string submoduleInfo() {
	//is just easier to read the text output that try to use git api to have a clean message
	static const QVector<QByteArray> stopWords{"Entering", "Entrando"};
	std::string                      final;
	QFile                            submoduleInfo(":/submoduleInfo");
	submoduleInfo.open(QFile::ReadOnly);
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
	//We probably ALWAYS use curl
	curl_global_init(CURL_GLOBAL_ALL);
	loadBuffer();
	//Maybe not the best thing, but no idea how else to achieve a better globalish

	config = _config;
	//We are server side we do not care about human broken standard
	std::setlocale(LC_ALL, "C");
	//Also for Qt for translation ecc
	//QLocale::setDefault(QLocale::C);
	QLocale l(QLocale::C, QLocale::UnitedStates);
	QLocale::setDefault(l);
	//If EaRTh iS FLAAAT why timezone ?!11!!?
	setenv("TZ", "UTC", 1);
	tzset();

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
	header += fmt::format("{0:{2}}STARTED:{0:{3}}{1} UTC\n", "", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss"), initialSpace, lineLenght - initialSpace - 31);
	header += fmt::format("{0:{3}}PID:{1: >{2}}\n", "", QCoreApplication::applicationPid(), lineLenght - initialSpace - 4, initialSpace);
	header += "\n";
	header += fmt::format("{0:{1}}GIT_MODULES:\n", "", initialSpace);
	header += submoduleInfo();
	header += "\n";
	header += fmt::format("{0:{2}}{0:*^{1}}", "", lineLenght - initialSpace, initialSpace);
	header += "\x1B[0m";
	header += "\n\n";
	fmt::print(fmt::runtime(header));
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

	qInstallMessageHandler(generalMsgHandler);
}

//QDebug send in stderr, but we want to use stdout
void generalMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
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
	switch (type) {
	case QtDebugMsg:
	case QtInfoMsg:
		stream = stdout;
		break;
	case QtCriticalMsg:
		[[fallthrough]];
	case QtFatalMsg:
		callViaTwilio();
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

		{
			QString msg2slack = QSL("<@U93PHQ62J> ") + warningHeader1 + QSL("\n") + warningHeader2 + QSL("\n\n") + msg;
			sendSlack(msg2slack, config->slackOpt.warningChannel);
		}
		{
			// subject
			auto subject = QSL("Error from %1 @ %2 in %3").arg(QCoreApplication::applicationName(), config->instanceName, funkz);
			// message
			auto warningMessage = warningHeader1 + QSL("<br/>") + warningHeader2 + QSL("<br/><br/><pre>") + msg + "</pre>";
			sendMail(subject, warningMessage);
		}

		stream = stderr;
		break;
	}

	auto msgFinal = localMsg.constData();

	fprintf(stream, "%s %s:%u (%s)\n%s\n----------\n", time.toUtf8().constData(), file, context.line, funkz, msgFinal);
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
