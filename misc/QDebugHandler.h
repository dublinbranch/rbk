#pragma once
#include "QDebugConfig.h"
#include <QDebug>
#include <chrono>
#include <string_view>

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
void shutdownQtLogging();

/** Append line to the disk error log (log/<time>.err) without waiting.
 * Returns false if the log lock is busy or the file is not open. For code that must not block,
 * like the forced-exit line of rbk::Shutdown. */
bool tryWriteDiskLog(std::string_view line);

/** Log one line like generalMsgHandler (stdout + log/<time>.log, or stderr + .err if error),
 * but wait at most `wait` for the log lock; return false if it stays busy. For rbk::Shutdown's
 * watcher, which must not wait for a thread that may be stuck while it logs. */
bool tryLogLine(bool error, std::string_view line, std::chrono::milliseconds wait = std::chrono::milliseconds(100));
void sendMail(QString subject, QString message);
void sendSlack(const QString& msg, std::string channel = "");
void callViaTwilio();
