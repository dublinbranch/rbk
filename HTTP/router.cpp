#include "router.h"
#include "PMFCGI.h"
#include "beastConfig.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/caching/apcu2.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/thread/tmonitoring.h"
#include <QDebug>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <chrono>
#include <cstdlib>
#include <map>
#include <rbk/fmtExtra/customformatter.h>
#include <string>

using namespace std;

class Echo : public RequestBase {
      public:
	virtual shared_ptr<RequestBase> create() const override {
		return make_shared<Echo>();
	}

	Payload immediate(PMFCGI& status) override {
		Payload payload;
		payload.html = status.serialize();
		return payload;
	}
};

class Status : public RequestBase {
      public:
	virtual shared_ptr<RequestBase> create() const override {
		return make_shared<Status>();
	}

	// for local testing
	// 127.0.0.1:1984/Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww
	Payload immediate(PMFCGI& status) override {
		(void)status;
		Payload payload;
		payload.html = composeStatus();

		payload.html += "<h1>Cache Info</h1>\n";
		payload.html += APCU::getInstance()->info();

		payload.mime = "text/html";
		return payload;
	}
};

// functor map

mapV2<std::string, RequestBase*> getDefaultRouting() {
	return {{

	    {"echo", new Echo},
	    {"Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww", new Status}

	}};
}

Payload Router::immediate(PMFCGI& status, const BeastConf* conf) {
	Url  url(status.path);
	auto path = url.url.path().toStdString().substr(1);

	//	if (isUserAgentBlacklisted(dk.originalInvocationUrl)) {
	//		Payload p;
	//		p.html       = "ok";
	//		p.statusCode = 200;
	//		return p;
	//	}

	if (auto v = conf->routing.get(path); v) {
		//Single catch point in case of fatal excpetion
		Payload p;
		try {
			//readDebugParam();
			// inside obj is a ptr, but also the value, so is a **
			operation = (*(v.val))->create();
			p         = operation->immediate(status);

		} catch (std::exception& e) {
			//addFlag(dk.errorCode, DkError::minorException);
			//exception type will be preserved
			throw;
		}
		//dk.processingEnd.setNow();
		return p;
	}
	Payload p;
	p.html       = fmt::format("invalid path >>> {} <<< no routing available", path);
	p.statusCode = 400;
	return p;
}

//bool Router::isUserAgentBlacklisted(const Url& url) {
//	const auto& parameters = url.query;
//	QString     userAgent;
//	parameters.swap("HTTP_USER_AGENT", userAgent);
//	return conf().userAgentsBlacklist.contains(userAgent);
//}

void Router::deferred() {
	if (operation) {
		operation->deferred();
	}
}
