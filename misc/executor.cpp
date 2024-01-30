#include "executor.h"

#include "echo.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/log/log.h"
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QProcess>

#include "rbk/QStacker/qstacker.h"

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
	process.waitForFinished(static_cast<int>(opt.maxTimeInS) * 1000);
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
