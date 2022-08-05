#ifndef MAILFETCHER_H
#define MAILFETCHER_H

#include "mincurl.h"
#include <QString>
#include <vector>

struct MailFetcherConfig {
	QString username;
	QString password;
	// imap query to search the mail that will be downloaded
	QString searchQuery;
	QString folderUrl;

	bool    logExecution = false;
	QString logFile;

	bool    skipSslVerification                = false;
	bool    moveToProcessedFolderAfterDownload = false;
	QString processedFolderName;
};

struct Mail {
	// id on mail server
	int64_t    id = 0;
	QByteArray content;
	bool       ok = false;
};

class MailFetcher {
      public:
	MailFetcher(MailFetcherConfig conf) {
		this->config = conf;
	}
	std::vector<Mail> fetch(bool verbose, bool printError);

      private:
	CurlKeeper        curl;
	MailFetcherConfig config;

	void             logSearchQuery(QString _username, QString _password, QString _folderUrl, QString _searchQuery);
	void             logSearchResponse(QString curlCode, QString curlError, QString result);
	void             logMailQuery(QString mailUrl);
	void             logMailResponse(QString curlCode, QString curlError, QString mail);
	std::vector<int> getMailIds(bool printError);
	QByteArray       getRawMail(int mailId, bool verbose, bool printError);
	bool             copyMailOnServer(int mailId);
	bool             moveMailOnServer(int mailId);
	bool             deleteMailOnServer(int mailId);
};

#endif // MAILFETCHER_H
