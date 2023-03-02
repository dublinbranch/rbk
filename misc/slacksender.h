#ifndef SLACKSENDER_H
#define SLACKSENDER_H

#include <QString>

class SlackSender {
      public:
	/**
	 * @brief SlackSender
	 * @param prefix with # for channel @ for user
	 * @param coolDown
	 */
	SlackSender(const QString& _channel, uint32_t _coolDown = 30);
	SlackSender(uint32_t _coolDown);

	void sendSlackMessage(QString msg);

	static void sendAsync(const QString& channel, const QString& msg);
	static void sendToDestSlackChannel(QString msg);

	void setCoolDown(const uint32_t& value);

	//needed as std::thread does not accept polymorphic
	static void sendQ16(const QString& channel, const QString& msg);

	//this will just send the message regardless of cooldown ecc, use with caution!
	static void send(const QString& channel, const QString& msg);
	static void send(const std::string& channel, const std::string& msg);

      private:
	QString channel;
	//how much time between one send and the other, to avoid needless spam, 30 sec is the default internal hard coded limit is 10
	static constexpr uint32_t minCooldown = 10;
	uint32_t                  coolDown    = 30;
};

#endif // SLACKSENDER_H
