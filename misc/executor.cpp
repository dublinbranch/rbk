#include "executor.h"

#include "echo.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/log/log.h"
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QProcess>

#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"

using namespace std;

bool Execute_logStackTrace = true;
bool Execute_printOnError  = false;

void readData(QProcess& process, QProcess::ProcessChannel channel, QByteArray& buffer) {
	process.setReadChannel(channel);
	while (true) {
		auto data = process.readLine(0);
		if (data.isEmpty()) {
			break;
		}
		buffer.append(data);
		//echo(data);
	}
}

Log execute(QStringList& cmd, ExecuteOpt opt) {
	Log      log;
	QProcess process;
	auto     env = QProcessEnvironment::systemEnvironment();

	QObject::connect(&process, &QProcess::readyReadStandardError, [&process, &log]() {
		readData(process, QProcess::StandardError, log.stdErr);
	});
	QObject::connect(&process, &QProcess::readyReadStandardOutput, [&process, &log]() {
		readData(process, QProcess::StandardOutput, log.stdOut);
	});

	env.remove("LD_LIBRARY_PATH");
	process.setProcessEnvironment(env);
	auto task = cmd.takeFirst();
	process.start(task, cmd);
	if (!process.waitForStarted(1000)) {
		log.category = Log::Error;
		log.stdErr   = "process did not start after 1000ms";
		return log;
	}

	{
		auto wait = static_cast<int>(opt.maxTimeInS) * 1000;
		//this is so retarded, for REASON this is not actually wait in case the process do not exists
		usleep(100 * 1000); //100ms;
		process.waitForFinished(wait);
	}
	log.setEnd();

	log.section = task + " " + cmd.join(" ");
	readData(process, QProcess::StandardError, log.stdErr);
	readData(process, QProcess::StandardOutput, log.stdOut);

	if (Execute_logStackTrace) {
		log.stackTrace = QStacker();
	}

	if (!opt.isRetarded) {
		if (log.stdErr.isEmpty()) {
			//at this stage I do not know if a program output is info or warning here
			log.category = Log::Info;
		} else {
			log.category = Log::Error;
			if (Execute_printOnError) {
				qCritical().noquote() << F16("For {} \n stdlog: {}\n stderr: {} \n Trace: {}",
				                             cmd[1], log.stdOut, log.stdErr, log.stackTrace);
			}
		}
	}

	return log;
}

Log execute(const QString& cmd, ExecuteOpt opt) {
	auto param = cmd.split(" ");
	return execute(param, opt);
}

Log sudo(const QStringViewV2& cmd, ExecuteOpt opt) {
	QStringList pack;
	pack << "/bin/bash";
	pack << "-c"
	     << "sudo " + cmd;
	return execute(pack, opt);
}

Log saveInto(const QStringViewV2& path, const QByteViewV2& content, QString chown, QString chmod) {
	Log log;
	log.section = "saveInto";

	auto temp = getTempFile(QString{});

	filePutContents(content, temp, true);
	log.push(sudo(F16("mv {} {}", temp, path)));
	log.push(sudo(F16("chown {} {}", chown, path)));
	log.push(sudo(F16("chmod {} {}", chmod, path)));

	return log;
}

Log moveInto(const QString& old, const QString& neu, QString chown, QString chmod) {
	Log log;
	log.section = __FUNCTION__;

	log.push(sudo(F16("mv {} {}", old, neu)));
	log.push(sudo(F16("chown {} {}", chown, neu)));
	log.push(sudo(F16("chmod {} {}", chmod, neu)));

	return log;
}

Log copyInto(const QStringViewV2& old, const QStringViewV2& neu, QString chown, QString chmod) {
	Log log;
	log.section = __FUNCTION__;

	log.push(sudo(F16("cp {} {}", old, neu)));
	log.push(sudo(F16("chown {} {}", chown, neu)));
	log.push(sudo(F16("chmod {} {}", chmod, neu)));

	return log;
}
