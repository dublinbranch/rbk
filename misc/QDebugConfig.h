#pragma once
#include <QString>
#include <optional>
#include <vector>

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
	std::optional<QString>   instanceName;
};
