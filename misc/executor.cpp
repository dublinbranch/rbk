#include "executor.h"

#include "echo.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/log/log.h"
#include <QDateTime>
#include <QDebug>

#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

using namespace std;

bool Execute_logStackTrace = true;
bool Execute_printOnError  = false;

Log execute(const std::vector<string>& args, ExecuteOpt opt) {
	Log log;
	log.section = F16("{} {}", args.front(), fmt::join(args, ","));

	reproc::process process;
	reproc::options options;
	options.deadline = reproc::milliseconds((int)(opt.maxTimeInS * 1000));

	std::error_code ec = process.start(args, options);

	//we are not going to write any data, plus if you do not do this LXC will hang!!!
	process.close(reproc::stream::in);

	if (ec == std::errc::no_such_file_or_directory) {
		throw ExceptionV2(F("Program >>>{}<<< not found. Make sure it's available from the PATH.", args.front()));
	} else if (ec) {
		log.stdErr = F8("{} {}", ec.message(), ec.value());
		return log;
	}
	std::string          output;
	reproc::sink::string sink(output);

	std::string          error;
	reproc::sink::string sinkErr(error);

	ec = reproc::drain(process, sink, sinkErr);

	if (!output.empty()) {
		log.stdOut = QByteArray::fromStdString(output);
	}

	if (ec) {
		log.stdErr.append(F8("\n{} {}\n", ec.message(), ec.value()));
	}

	if (!error.empty()) {
		log.stdErr.append(QByteArray::fromStdString(error));
	}

	log.setEnd();

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
				                             args.front(), log.stdOut, log.stdErr, log.stackTrace);
			}
		}
	}

	return log;
}

Log execute(const QStringAdt& cmd, ExecuteOpt opt) {
	auto                param = cmd.split(" ");
	std::vector<string> args;
	for (auto& p : param) {
		args.push_back(p.toStdString());
	}
	return execute(args, opt);
}

Log sudo(const StringAdt& cmd, ExecuteOpt opt) {
	std::vector<string> pack;
	pack.push_back("sudo");
	pack.push_back(cmd);
	return execute(pack, opt);
}

Log saveInto(const QStringAdt& path, const QByteAdt& content, QString chown, QString chmod) {
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

Log copyInto(const QStringAdt& old, const QStringAdt& neu, QString chown, QString chmod) {
	Log log;
	log.section = __FUNCTION__;

	log.push(sudo(F16("cp {} {}", old, neu)));
	log.push(sudo(F16("chown {} {}", chown, neu)));
	log.push(sudo(F16("chmod {} {}", chmod, neu)));

	return log;
}
