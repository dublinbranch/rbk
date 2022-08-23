#ifndef SLACKSENDER_H
#define SLACKSENDER_H

#include <QString>

enum class Channel : int {
	acc_mgrs_techads = 0,
	dev,
	dev_backend,
	dev_frontend,
	develop_test,
	error_ext_agency,
	error_기대해,
	error_ocode,
};

QString toString(Channel channel);

class SlackSender {
      public:
	/**
	 * @brief SlackSender
	 * @param prefix with # for channel @ for user
	 * @param coolDown
	 */
	SlackSender(Channel _channel, uint32_t _coolDown = 30);
	SlackSender(uint32_t _coolDown);

	void sendSlackMessage(QString msg);

	static void sendAsync(Channel channel, const QString& msg);
	static void sendToDestSlackChannel(QString msg);

	void setChannel(const Channel& value);

	void setCoolDown(const uint32_t& value);

      private:
	Channel channel;
	//how much time between one send and the other, to avoid needless spam, 30 sec is the default internal hard coded limit is 10
	static constexpr uint32_t minCooldown = 10;
	uint32_t                  coolDown    = 30;

	//this will just send the message regardless of cooldown ecc, use with caution!
	static void send(Channel channel, const QString& msg);
};

// destination channel for SlackSender::send()
// default is channel error
inline thread_local Channel destSlackChannel = Channel::error_기대해;

#endif // SLACKSENDER_H
