#pragma once
#include "QDebugConfig.h"
#include <QDebug>

void initLocaleTZ();
void commonInitialization(const NanoSpammerConfig* _config);

//use like
//qInstallMessageHandler(generalMsgHandler);

void generalMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void lowSpamMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void sendMail(QString subject, QString message);
void sendSlack(const QString& msg, std::string channel="");
void callViaTwilio();
