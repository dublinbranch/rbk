#include "beastConfig.h"
#include "Payload.h"
#include "rbk/filesystem/filefunction.h"

#include <exception>

void BeastConf::setBasePath(const std::string& newBasePath) {
	basePath = newBasePath;
}

bool applyErrorAnswer(const BeastConf* conf, const PMFCGI& status, Payload& payload, ErrorKind kind,
                      unsigned statusCode, const std::string& message) noexcept {
	if (!conf || !conf->errorAnswer) {
		return false;
	}
	// as the other request hooks: a broken hook leaves a line, and rbk's answer
	const auto failed = [conf](std::string_view what) noexcept {
		try {
			fileAppendContents("\n------\n errorAnswer failed: " + std::string(what), conf->logFolder + "/stdException.log");
		} catch (...) {
		}
	};
	try {
		auto answer = conf->errorAnswer(status, kind, statusCode, message);
		if (!answer) {
			return false;
		}
		// only moves from here: nothing can throw half way
		payload.statusCode = answer->statusCode;
		payload.mime       = std::move(answer->mime);
		payload.html       = std::move(answer->body);
		return true;
	} catch (const std::exception& e) {
		failed(e.what());
		return false;
	} catch (...) {
		failed("unknown exception");
		return false;
	}
}
