#pragma once
#include <QDateTime>
#include <QString>
#include <optional>
#include <vector>

inline constexpr int kDefaultMailCooldownSec = 8 * 3600;

struct SlackOpt {
	bool        warningON = true;
	std::string warningChannel;
	//FIXME why is slackAPIToken not here ????
};

struct NanoSpammerConfig {
	SlackOpt slackOpt;
	// if true then a call is made to report a QtFatalMsg
	bool                     BRUTAL_INHUMAN_REPORTING = false;
	bool                     warningToMail            = true;
	std::vector<std::string> warningMailRecipients    = {"admin@seisho.us"};
	// Min seconds between two warning mails from the same file:line. 0 = no limit.
	// Optional so old config.json files without the key still load.
	std::optional<int>       mailCooldownSec          = kDefaultMailCooldownSec;
	QString                  instanceName             = "REPLACE ME";
	QDateTime                startedAt;
};
