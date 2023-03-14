#include "slacksender.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/minCurl/mincurl.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <mutex>
#include <thread>

using namespace std::string_literals;

//#include "rbk/misc/slacksender.h"
//SlackSender::send("#backenderror_afd", "prova");
//something like xoxb-xxxxxxxxxxxxxxxxxxxxxxxxx
//https://api.slack.com/web#basics
extern const char* slackAPIToken;
const char*        slackAPIToken = nullptr;

SlackSender::SlackSender(const QString& channel_, uint32_t coolDown_) {
	channel = channel_;
	setCoolDown(coolDown_);
}

SlackSender::SlackSender(uint32_t _coolDown) {
	setCoolDown(_coolDown);
}

/**
 * @brief sendSlackMessage
 * @param msg
 * @param red bypass the anti spam
 */
void SlackSender::sendSlackMessage(QString msg) {
	static std::mutex mu;

	std::unique_lock<std::mutex> lock(mu, std::try_to_lock);
	if (!lock.owns_lock()) {
		//too much concurret request to call ? this should not happen! too much spam in any case is running
		return;
	}

	static int64_t lastCall = 0;
	auto           now      = QDateTime::currentSecsSinceEpoch();
	uint32_t       next     = lastCall + coolDown;
	if (next > now) { //if still in cooldown
		return;
	} else {
		lastCall = now;
	}

	//Else we can easily miss a good amount of stack trace
	int const maxMsgLength = 1024;
	if (msg.length() > maxMsgLength) {
		msg = msg.left(maxMsgLength);
	}

	QJsonObject json;
	json["text"]    = msg;
	json["channel"] = channel;
	QJsonDocument jDoc(json);

	CurlKeeper curl;
	CurlHeader h;
	h.add("Authorization: Bearer "s + slackAPIToken);
	h.add("Content-type: application/json");
	h.set(curl);
	auto res = urlPostContent("https://slack.com/api/chat.postMessage", jDoc.toJson(), false, curl);
	{
		auto doc = QJsonDocument::fromJson(res.result);
		if (doc["ok"].toBool() != true) {
			fprintf(stderr, "slack error %s", res.result.constData());
		}
	}
}

//https://discuss.newrelic.com/t/sending-alerts-to-slack-with-channel-notification/35921/3
//https://api.slack.com/reference/surfaces/formatting#mentioning-users
//https://help.workast.com/hc/en-us/articles/360027461274-How-to-find-a-Slack-user-ID
void SlackSender::send(const QString& channel, const QString& msg) {
	SlackSender slack(channel, minCooldown);
	slack.sendSlackMessage(msg);
}

void SlackSender::send(const std::string& channel, const std::string& msg) {
	send(QString::fromStdString(channel), QString::fromStdString(msg));
}

void SlackSender::sendAsync(const QString& channel, const QString& msg) {
	std::thread send(SlackSender::sendQ16, channel, msg);
	//In theory we should register this one, but the program wil not terminate immediately, and in any case the errore is collected and send via mail outside of it
	send.detach();
}

void SlackSender::sendToDestSlackChannel(QString msg) {
	SlackSender slack(minCooldown);
	slack.sendSlackMessage(msg);
}

void SlackSender::setCoolDown(const uint32_t& value) {
	if (value < minCooldown) {
		qDebug().noquote() << "minimum slack spam time is 10 second... no one is going to read all of that spam in any case, is just useless..." + QStacker16Light();
		coolDown = minCooldown;
		return;
	}
	coolDown = value;
}

void SlackSender::sendQ16(const QString& channel, const QString& msg) {
	return send(channel, msg);
}
