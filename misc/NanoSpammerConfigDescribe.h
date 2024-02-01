#ifndef NANOSPAMMERCONFIGDESCRIBE_H
#define NANOSPAMMERCONFIGDESCRIBE_H

#include "QDebugConfig.h"
#include "boost/json/fwd.hpp"
#include <boost/describe.hpp>

BOOST_DESCRIBE_STRUCT(NanoSpammerConfig, (), (slackOpt, BRUTAL_INHUMAN_REPORTING, warningToMail, warningMailRecipients, instanceName))
BOOST_DESCRIBE_STRUCT(SlackOpt, (), (warningON, warningChannel))

#endif // NANOSPAMMERCONFIGDESCRIBE_H
