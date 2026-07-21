#pragma once
#include "QDebugConfig.h"
#include <QDebug>

void initLocaleTZ();
void commonInitialization(const NanoSpammerConfig* _config);

/** True when rbk was built with RBK_WITH_MINCURL (defines useMinCurl). */
bool hasCurlSupport();

/** Fail early if warningToMail is on but curl was not compiled into rbk. */
void requireCurlIfWarningMailEnabled(const NanoSpammerConfig& spamConf);

//use like
//qInstallMessageHandler(generalMsgHandler);

void generalMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void lowSpamMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void sendMail(QString subject, QString message);
void sendSlack(const QString& msg, std::string channel="");
void callViaTwilio();
