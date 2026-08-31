#include "router.h"
#include "PMFCGI.h"
#include "beastConfig.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/HTTP/mime.h"
#include "rbk/HTTP/staticPath.h"
#include "rbk/HTTP/url.h"
#include "rbk/misc/escapeH.h"
#include "rbk/caching/apcu2.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/thread/tmonitoring.h"
#ifdef WITH_Jemalloc
#include "rbk/jemalloc/jemutil.h"
#endif
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

	void immediate(PMFCGI& status, Payload& payload) override {
		payload.html = status.serialize();
	}
};

class Status : public RequestBase {
      public:
	virtual shared_ptr<RequestBase> create() const override {
		return make_shared<Status>();
	}

	// for local testing
	// 127.0.0.1:1984/Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww
	void immediate(PMFCGI& status, Payload& payload) override {
		(void)status;

		payload.html = composeStatus();

		payload.html += "<h1>Cache Info</h1>\n";
		payload.html += APCU::getInstance()->info();

		payload.mime = "text/html; charset=utf-8";
	}
};

#ifndef WITH_Jemalloc
namespace {
void jemallocNotBuilt(Payload& payload) {
	payload.statusCode = 501;
	payload.mime       = "text/plain; charset=utf-8";
	payload.html       = "jemalloc is not available in this build\n";
}
} // namespace
#endif

class JemallocPage : public RequestBase {
      public:
	virtual shared_ptr<RequestBase> create() const override {
		return make_shared<JemallocPage>();
	}

	// unguessable — 127.0.0.1:8082/ziAvks8rGUyQcoMU5K3eC9QsUvYv0lnK5iv7d8wdBWb7WAoa
	void immediate(PMFCGI& status, Payload& payload) override {
		(void)status;
#ifdef WITH_Jemalloc
		payload.html = JEMUtil::pageHtml();
		payload.mime = "text/html; charset=utf-8";
#else
		jemallocNotBuilt(payload);
#endif
	}
};

class JemallocStats : public RequestBase {
      public:
	virtual shared_ptr<RequestBase> create() const override {
		return make_shared<JemallocStats>();
	}

	void immediate(PMFCGI& status, Payload& payload) override {
		(void)status;
#ifdef WITH_Jemalloc
		payload.html = JEMUtil::statsPrint();
		payload.mime = "text/plain; charset=utf-8";
#else
		jemallocNotBuilt(payload);
#endif
	}
};

void Router::immediate(PMFCGI& status, const BeastConf* conf, Payload& payload) {
	//take out initial /
	auto path = status.url.url.path().toStdString();
	if (path.empty()) {
		payload.statusCode = 404;
		return;
	}
	path = path.substr(1);

	//	if (isUserAgentBlacklisted(dk.originalInvocationUrl)) {
	//		Payload p;
	//		p.html       = "ok";
	//		p.statusCode = 200;
	//		return p;
	//	}

	if (auto v = conf->routing.get(path); v) {
		//Single catch point in case of fatal excpetion
		try {
			//readDebugParam();
			// inside obj is a ptr, but also the value, so is a **
			operation = (*(v.val))->create();
			operation->immediate(status, payload);
			return;
		} catch (std::exception& e) {
			//addFlag(dk.errorCode, DkError::minorException);
			//exception type will be preserved
			throw;
		}
		//dk.processingEnd.setNow();
	} else if (auto v2 = conf->routingSimple.get(path); v2) {
		(*(v2.val))(status, payload);
		return;
	} else if (conf->staticFile.has_value() && !conf->staticFile.value().empty()) {
		if (auto p = resolveStaticFile(conf->staticFile.value(), path)) {
			auto res = fileGetContents2(*p, true, 0);
			if (res.exist) {
				payload.html       = res.content.toStdString();
				payload.mime       = getMimeType(path);
				payload.statusCode = 200;
				payload.setCacheHeader(conf->staticFileCacheTTL);
				return;
			}
		}
	}
	payload.mime       = "text/plain; charset=utf-8";
	payload.html       = fmt::format("invalid path >>> {} <<< no routing available", escapeH(path));
	payload.statusCode = 400;
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

// functor map

mapV2<std::string, RequestBase*> getDefaultRouting() {
	return {{

	    {"echo", new Echo},
	    {"Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww", new Status},
	    {jemallocPagePath, new JemallocPage},
	    {jemallocStatsPath, new JemallocStats}

	}};
}

mapV2<string, SimpleRoutedType> getDefaultSimpleRouting() {
	return {};
}
