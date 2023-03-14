#pragma once
#include <QString>
#include <vector>

struct SlackOpt {
	bool        warningON      = true;
	std::string warningChannel = "";
};

struct NanoSpammerConfig {
	SlackOpt slackOpt;
	// if true then a call is made to report a QtFatalMsg
	bool                    BRUTAL_INHUMAN_REPORTING = false;
	bool                    warningToMail            = true;
	std::vector<QByteArray> warningMailRecipients    = {"admin@seisho.us", "claudio@tech.techadsmedia.com"};
	QString                 instanceName;
};
