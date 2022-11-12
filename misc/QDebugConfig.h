#pragma once
#include <QString>
#include <vector>

struct NanoSpammerConfig {
	bool                    BRUTAL_INHUMAN_REPORTING = true;
	bool                    warningToSlack           = true;
	bool                    warningToMail            = true;
	bool                    useTestSlackChannel      = false;
	std::vector<QByteArray> warningMailRecipients    = {"admin@seisho.us", "claudio@tech.techadsmedia.com"};
	QString                 instanceName;
};
