#ifndef MAILFETCHER_H
#define MAILFETCHER_H

#include "mincurl.h"
#include <QString>
#include <vector>

struct MailFetcherConfig {
	std::string username;
	std::string password;
	// imap query to search the mail that will be downloaded
	std::string searchQuery;
	std::string folderUrl;

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

	static void        extractAttachmentsFromFile(const QString& rawMailFileName, const QString& outputFolderName);
	static QStringList extractAttachmentsFromBuffer(const QByteArray& buffer, const QString& outputFolderName);

      private:
	CurlKeeper        curl;
	MailFetcherConfig config;

	void             logSearchQuery();
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
