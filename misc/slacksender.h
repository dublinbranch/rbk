#ifndef SLACKSENDER_H
#define SLACKSENDER_H

#include "misc/intTypes.h"
#include <QString>

class SlackSender {
      public:
	/**
	 * @brief SlackSender
	 * @param prefix with # for channel @ for user
	 * @param coolDown
	 */
	SlackSender(const QString& _channel, u64 _coolDown = 30);
	explicit SlackSender(u64 _coolDown);

	void sendSlackMessage(QString msg);

	static void sendAsync(const std::string& channel, const QString& msg);
	static void sendAsync(const QString& channel, const QString& msg);
	static void sendToDestSlackChannel(QString msg);

	void setCoolDown(const u64& value);

	//needed as std::thread does not accept polymorphic
	static void sendQ16(const QString& channel, const QString& msg);

	//this will just send the message regardless of cooldown ecc, use with caution!
	static void send(const QString& channel, const QString& msg);
	static void send(const std::string& channel, const std::string& msg);

      private:
	QString channel;
	//how much time between one send and the other, to avoid needless spam, 30 sec is the default internal hard coded limit is 10
	static constexpr u64 minCooldown = 10;
	u64                  coolDown    = 30;
};

#endif // SLACKSENDER_H
