#ifndef BEASTCONFIG_H
#define BEASTCONFIG_H

#include "rbk/mapExtensor/mapV2.h"
class RequestBase;

class PMFCGI;
class Payload;

struct BeastConf {

	std::function<void(PMFCGI& status, Payload& payload)> prePhase1  = nullptr;
	//std::function<void()> post = nullptr;

	using SimpleRoutedType = void (*)(PMFCGI& status, Payload& payload);

	mapV2<std::string, SimpleRoutedType> routingSimple;
	mapV2<std::string, RequestBase*>     routing;
	mapV2<std::string, std::string>      defaultHeader;

	std::string logFolder;

	int         worker  = 1;
	std::string address = "127.0.0.1";
	ushort      port    = 8081;
};

#endif // BEASTCONFIG_H
