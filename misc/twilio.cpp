#include "twilio.h"
#include "rbk/minCurl/curlpp.h"
#include "rbk/minCurl/mincurl.h"
#include <QDateTime>
#include <mutex>

extern const char* twilioAPIurl;
const char*        twilioAPIurl = nullptr;

void Twilio::call() {
	Twilio t;
	t.callInner();
}

void Twilio::callInner() {
	static std::mutex mu;

	std::unique_lock<std::mutex> lock(mu, std::try_to_lock);
	if (!lock.owns_lock()) {
		//too much concurret request to call ? this should not happen! too much spam in any case is running
		return;
	}

	static int64_t lastCall = 0;

	auto now = QDateTime::currentSecsSinceEpoch();
	if ((now - lastCall) < 180) { //quite useless to spam call
		return;
	}
	lastCall = now;
	//recover token is 6wy-uFOUiBRn_4VAVgJvIaWkrp4qaIZQfvhjXIwx
	CURLpp Marx = CURLpp::Builder()
	                  .set_timeout(3000)
	                  .set_url("https://api.twilio.com/2010-04-01/Accounts/ACb24c402679b0c080f7ea37a1253e32f5/Calls")
	                  .set_verbose(false)
	                  .set_post(1)
	                  .set_post_fields(twilioAPIurl)
	                  .set_auth("ACb24c402679b0c080f7ea37a1253e32f5:38babc2909f0e50ae9e2f3db5a076800")
	                  .build();
	auto res = Marx.perform();
	if (res.find("")) {
	}
}
