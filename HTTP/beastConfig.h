#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H

#include "rbk/mapExtensor/mapV2.h"
#include <filesystem>
class RequestBase;

class PMFCGI;
class Payload;

struct BeastConf {
      public:
	friend class PMFCGI;

	//We must use std::function and not plain functor, to be able to use lambdas, but in this case is probably overkill...

	//This is done immediately after loading the bare data (cookie, get)
	std::function<void(PMFCGI& status, Payload& payload)> prePhase1 = nullptr;

	std::function<bool(PMFCGI& status, Payload& payload)> loginManager = nullptr;

	//This is done AFTER the loginManager
	std::function<bool(PMFCGI& status, Payload& payload)> common1 = nullptr;

	//we can override the function that return the base path to be used, sometimes we have to mix stuff
	std::function<std::string(const PMFCGI* status)> basePathFunctor;

	using SimpleRoutedType = void (*)(PMFCGI& status, Payload& payload);

	mapV2<std::string, SimpleRoutedType> routingSimple;
	mapV2<std::string, RequestBase*>     routing;
	mapV2<std::string, std::string>      defaultHeader;

	//if SET in case a path IS NOT MAPPED it will be searched on DISK inside this PROGRAM RELATIVE folder
	std::filesystem::path staticFile;
	//This will set the expire tag, like in NGINX if you do location ~* \.(js|css|png|jpg|jpeg|gif|ico)$ { expires XX;
	uint staticFileCacheTTL = 0;

	std::string logFolder = "httpLog";

	int         worker  = 1;
	std::string address = "127.0.0.1";
	ushort      port    = 8081;
	//we do normally ONLY print the HTTPException, but in some case of self contained system is ok to print all
	bool htmlAllException = false;

	/**
	 * @brief logRequest should be set true in case we are NOT running under a webserver (that is already logging)
	 */
	bool logRequest = false;
	/**
	 * @brief logResponse will enable saving the FULL html response to the log file,
	 * this if uncheched will enormously inflate the log file, you normally want to use
	 * with the whitelisted block to allow ONLY certain path to be logged
	 */
	bool logResponse = false;

	/**
	 * @brief logRequestByIp ^_^
	 */
	bool logRequestByIp = false;

	/**
	 * @brief maxResponseSize sometimes you do error, and do not want to clog the response too much
	 * most of the important data are quite small json 99& of the times
	 */
	size_t maxResponseSize = 1024 * 16;

	/**
	 * @brief those list are simple pattern matching NOT REGEX
	 * white is processed first, than black can override and block
	 */
	std::vector<std::string> logWhitelist;
	std::vector<std::string> logBlacklist;
	void                     setBasePath(const std::string& newBasePath);

	std::optional<std::string> basePath;

      private:
};

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
