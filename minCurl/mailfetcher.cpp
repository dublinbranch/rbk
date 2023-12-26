#include "mailfetcher.h"
#include "mincurl.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <curl/curl.h>

void MailFetcher::logSearchQuery() {
	if (!config.logExecution) {
		return;
	}

	auto msg = fmt::format(R"(
- request -
username = {}
password = {}
folderUrl = {}
searchQuery = {}
)",
	                       config.username, config.password, config.folderUrl, config.searchQuery);

	logWithTime(config.logFile, msg);
}

void MailFetcher::logSearchResponse(QString curlCode, QString curlError, QString result) {
	if (!config.logExecution) {
		return;
	}

	QString skel = R"(- response -
curlCode = %1
curlError = %2
result = %3
)";
	auto    msg  = skel
	               .arg(curlCode)
	               .arg(curlError)
	               .arg(result);
	logWithTime(config.logFile, msg);
}

void MailFetcher::logMailQuery(QString mailUrl) {
	if (!config.logExecution) {
		return;
	}

	QString skel = R"(- request -
mailUrl = %1
)";
	auto    msg  = skel
	               .arg(mailUrl);
	logWithTime(config.logFile, msg);
}

void MailFetcher::logMailResponse(QString curlCode, QString curlError, QString mail) {
	if (!config.logExecution) {
		return;
	}

	QString skel = R"(- response -
curlCode = %1
curlError = %2
mail (truncated) = %3
)";
	auto    msg  = skel
	               .arg(curlCode)
	               .arg(curlError)
	               .arg(mail.left(500));
	logWithTime(config.logFile, msg);
}

//SORT BY DESC = std::sort(mailIdList.begin(), mailIdList.end(), std::greater<int>());
std::vector<int> MailFetcher::getMailIds(bool printError) {
	CURLcode       res = CURLE_OK;
	CurlCallResult result;
	Mail           emptyMail;
	/*
	 * search query
	 * find the id of the mail we want to download
	 */
	logSearchQuery();

	char errbuf[CURL_ERROR_SIZE] = {0};

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_USERNAME, config.username.data());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password.data());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QBWriter);
	curl_easy_setopt(curl, CURLOPT_URL, config.folderUrl.data());
	curl_easy_setopt(curl, CURLOPT_VERBOSE, false);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, config.searchQuery.data()); /* Perform the fetch */

	if (config.skipSslVerification) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	}

	res = curl_easy_perform(curl);

	// response

	auto& r = result.result;

	logSearchResponse(asString(res), curl_easy_strerror(res), r);

	// check curl result
	if (res != CURLE_OK) {
		if (printError) {
			//TODO
			// se curl danno errori
			// mettere loop con N tentativi

			auto msg = fmt::format(R"(
curl_easy_perform() failed!
res = {}
curl string error = {}
username = {}
password = {}
folderUrl = {}
searchQuery = {}
)",

			                       asSWString(res),
			                       curl_easy_strerror(res),
			                       config.username,
			                       config.password,
			                       config.folderUrl,
			                       config.searchQuery);

			qWarning().noquote() << msg.data() << QStacker16Light();
		}
		return {};
	}

	r.replace("* SEARCH", "");
	r = r.trimmed();

	if (r.isEmpty()) {
		// no mails
		return {};
	}

	auto             stringIdList = r.split(' ');
	std::vector<int> mailIdList;
	for (auto& stringId : stringIdList) {
		mailIdList.push_back(stringId.toInt());
	}
	return mailIdList;
}

QByteArray MailFetcher::getRawMail(int mailId, bool verbose, bool printError) {
	auto mailUrl = fmt::format("{};MAILINDEX={}", config.folderUrl, mailId);

	CurlCallResult mailResult;
	char           errbuf[CURL_ERROR_SIZE] = {0};
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.data());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mailResult.result);

	auto res = curl_easy_perform(curl);

	// response

	auto        mail = mailResult.result;
	std::string msg;

	if (res != CURLE_OK) {
		msg = fmt::format("curl_easy_perform() failed: mailUrl = {}, curlError = {}, errBuff = {}",
		                  mailUrl,
		                  curl_easy_strerror(res), errbuf);

		if (printError) {
			qWarning().noquote() << msg.data() << QStacker16Light();
		}
		return QByteArray();
	} else {
		msg = fmt::format("mailUrl = {} , errBuff = {}", mailUrl, errbuf);
	}

	logMailResponse(asString(res), msg.data(), mail);

	if (verbose) {
		qDebug().noquote() << mail;
	}
	return mail;
}

bool MailFetcher::copyMailOnServer(int mailId) {
	CurlCallResult curlResult;
	auto           mailUrl = fmt::format("{};MAILINDEX={}", config.folderUrl, mailId);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.data());
	// copy command is:
	// COPY <mailId> <destinationFolder>
	// e.g.
	// COPY 1 FOLDER
	auto copyCommand = QSL("COPY %1 %2")
	                       .arg(mailId)
	                       .arg(config.processedFolderName);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, copyCommand.toLocal8Bit().data());

	auto res = curl_easy_perform(curl);

	auto ok = (res == CURLE_OK) and curlResult.result.isEmpty();
	return ok;
}

bool MailFetcher::deleteMailOnServer(int mailId) {
	CurlCallResult curlResult;
	auto           mailUrl = fmt::format("{};MAILINDEX={}", config.folderUrl, mailId);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.data());
	auto flagAsDeleted = QSL("STORE %1 +Flags \\Deleted")
	                         .arg(mailId);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, flagAsDeleted.toLocal8Bit().data());

	auto res = curl_easy_perform(curl);

	auto ok = res == CURLE_OK;

	CurlCallResult curlResult2;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult2.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.data());
	auto expungeCommand = QSL("EXPUNGE");
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, expungeCommand.toLocal8Bit().data());

	auto res2 = curl_easy_perform(curl);

	auto ok2 = (res2 == CURLE_OK) and curlResult2.result.isEmpty();

	auto ok3 = ok and ok2;
	return ok3;
}

bool MailFetcher::moveMailOnServer(int mailId) {
	if (!config.moveToProcessedFolderAfterDownload) {
		return true;
	}

	auto ok = copyMailOnServer(mailId);
	if (!ok) {
		// do not delete the original one because we were not able to copy it
		return false;
	}
	auto deleteOk = deleteMailOnServer(mailId);
	return deleteOk;
}

std::vector<Mail> MailFetcher::fetch(bool verbose, bool printError) {
	auto mailIdLists = getMailIds(printError);

	if (!mailIdLists.size()) {
		// no mails
		return {};
	}

	std::vector<Mail> returnMe;
	for (auto&& mailId : mailIdLists) {
		/*
		 * mail query
		 */
		auto mail = getRawMail(mailId, verbose, printError);
		if (mail.isEmpty()) {
			continue;
		}

		returnMe.push_back({mailId, mail, true});
		moveMailOnServer(mailId);
	}

	return returnMe;
}

void MailFetcher::extractAttachmentsFromFile(const QString& rawMailFileName, const QString& outputFolderName) {
	QProcess    process;
	QStringList params;
	params << "-i" << rawMailFileName;
	params << "-d" << outputFolderName;
	auto env = QProcessEnvironment::systemEnvironment();
	env.remove("LD_LIBRARY_PATH");
	process.setProcessEnvironment(env);
	process.start("ripmime", params);
	process.waitForFinished(9999999);
	QByteArray errorMsg = process.readAllStandardError();
	QByteArray msg      = process.readAllStandardOutput();

	if (process.error() != QProcess::ProcessError::UnknownError || !errorMsg.isEmpty() || !msg.isEmpty()) {
		//https://github.com/inflex/ripMIME
		qCritical().noquote() << "error invoking ripmime, are you sure it exist?\nto install see\nhttps://s22.trott.pw/dev_wiki/index.php?title=Ripmime" + errorMsg + msg + process.errorString();
		throw 1;
	}
}

QStringList MailFetcher::extractAttachmentsFromBuffer(const QByteArray& buffer, const QString& outputFolderName) {
	auto finalFolder = outputFolderName + "/" + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
	mkdir(outputFolderName);
	mkdir(finalFolder);
	auto fname = finalFolder + "/buffer";

	//todo creare una funzione che ritorna anche error message ecc
	filePutContents(buffer, fname);

	MailFetcher::extractAttachmentsFromFile(fname, finalFolder);

	unlink(fname.toStdString().c_str());

	QDir        directory(finalFolder);
	auto        files = directory.entryList(QDir::Files);
	QStringList ok;
	for (auto& file : files) {
		//those 2 are the readable mail message, one the html, one the plaintext
		if (file == "textfile1" || file == "textfile0") {
			continue;
		}
		file = finalFolder + "/" + file;
		ok.append(file);
	}
	return ok;
}

int64_t Mail::getArrivalTime() const {
	throw ExceptionV2("function NOT IMPLEMENTED");
	/**
	 * Something like
    Received: by 2002:a05:6400:22c3:0:0:0:0 with SMTP id x3csp400663ecf;
	    Thu, 15 Sep 2022 04:20:05 -0700 (PDT)
    So find Received: by and take two newline
	 */
	//	if (arrivalTime) {
	//		return arrivalTime;
	//	}
	auto temp  = content.indexOf("Received: by");
	auto start = content.indexOf('\n', temp);
	auto end   = content.indexOf('\n', start + 1);
	auto line  = content.mid(start, end - start).simplified().trimmed();
	//what is the name of such datetime format
	QDateTime::fromString(line, "");
}
