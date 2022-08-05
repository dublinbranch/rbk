#include "mailfetcher.h"
#include "mincurl.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include <QDebug>
#include <curl/curl.h>

void MailFetcher::logSearchQuery(QString _username, QString _password, QString _folderUrl, QString _searchQuery) {
	if (!config.logExecution) {
		return;
	}

	QString skel = R"(- request -
username = %1
password = %2
folderUrl = %3
searchQuery = %4
)";
	auto    msg  = skel
	               .arg(_username)
	               .arg(_password)
	               .arg(_folderUrl)
	               .arg(_searchQuery);
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

// "mailIdList" is returned in descending order, so that when we process it we start from the greatest id
std::vector<int> MailFetcher::getMailIds(bool printError) {
	CURLcode       res = CURLE_OK;
	CurlCallResult result;
	Mail           emptyMail;
	/*
	 * search query
	 * find the id of the mail we want to download
	 */
	logSearchQuery(config.username, config.password, config.folderUrl, config.searchQuery);

	char errbuf[CURL_ERROR_SIZE] = {0};

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_USERNAME, config.username.toLocal8Bit().data());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password.toLocal8Bit().data());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QBWriter);
	curl_easy_setopt(curl, CURLOPT_URL, config.folderUrl.toLocal8Bit().data());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, config.searchQuery.toLocal8Bit().data()); /* Perform the fetch */

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

			QString errSkel = R"(
curl_easy_perform() failed!
res = %1
curl string error = %2
username = %3
password = %4
folderUrl = %5
searchQuery = %6
)";
			auto    err     = errSkel
			               .arg(res)
			               .arg(curl_easy_strerror(res))
			               .arg(config.username)
			               .arg(config.password)
			               .arg(config.folderUrl)
			               .arg(config.searchQuery);
			qWarning().noquote() << err << QStacker16Light();
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
	std::sort(mailIdList.begin(), mailIdList.end(), std::greater<int>());
	return mailIdList;
}

QByteArray MailFetcher::getRawMail(int mailId, bool verbose, bool printError) {
	auto mailUrl = QSL("%1;MAILINDEX=%2")
	                   .arg(config.folderUrl)
	                   .arg(mailId);

	CurlCallResult mailResult;
	char           errbuf[CURL_ERROR_SIZE] = {0};
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.toLocal8Bit().data());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mailResult.result);

	auto res = curl_easy_perform(curl);

	// response

	auto    mail = mailResult.result;
	QString msg;

	if (res != CURLE_OK) {
		msg = QSL("curl_easy_perform() failed: mailUrl = %1, curlError = %2, errBuff = %3")
		          .arg(mailUrl)
		          .arg(curl_easy_strerror(res))
		          .arg(errbuf);

		if (printError) {
			qWarning().noquote() << msg << QStacker16Light();
		}
		return QByteArray();
	} else {
		msg = QSL("mailUrl = %1 , errBuff = %2").arg(mailUrl).arg(errbuf);
	}

	logMailResponse(asString(res), msg, mail);

	if (verbose) {
		qDebug().noquote() << mail;
	}
	return mail;
}

bool MailFetcher::copyMailOnServer(int mailId) {
	CurlCallResult curlResult;
	auto           mailUrl = QSL("%1;MAILINDEX=%2")
	                   .arg(config.folderUrl)
	                   .arg(mailId);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.toLocal8Bit().data());
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
	auto           mailUrl = QSL("%1;MAILINDEX=%2")
	                   .arg(config.folderUrl)
	                   .arg(mailId);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.toLocal8Bit().data());
	auto flagAsDeleted = QSL("STORE %1 +Flags \\Deleted")
	                         .arg(mailId);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, flagAsDeleted.toLocal8Bit().data());

	auto res = curl_easy_perform(curl);

	auto ok = res == CURLE_OK;

	CurlCallResult curlResult2;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResult2.result);
	curl_easy_setopt(curl, CURLOPT_URL, mailUrl.toLocal8Bit().data());
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
