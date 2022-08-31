#pragma once
#include "config.h"
#include <QDebug>

void commonInitialization(const NanoSpammerConfig* _config);

//use like
//qInstallMessageHandler(generalMsgHandler);

void generalMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void lowSpamMsgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
void sendMail(QString subject, QString message);
void sendSlack(const QString& msg);
void callViaTwilio();
