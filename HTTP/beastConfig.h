#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H

#include "rbk/mapExtensor/mapV2.h"
#include <filesystem>
class RequestBase;

class PMFCGI;
class Payload;

struct BeastConf {

	std::function<void(PMFCGI& status, Payload& payload)> prePhase1 = nullptr;
	//std::function<void()> post = nullptr;

	using SimpleRoutedType = void (*)(PMFCGI& status, Payload& payload);

	mapV2<std::string, SimpleRoutedType> routingSimple;
	mapV2<std::string, RequestBase*>     routing;
	mapV2<std::string, std::string>      defaultHeader;

	//if SET in case a path IS NOT MAPPED it will be searched on DISK inside this PROGRAM RELATIVE folder
	std::filesystem::path staticFile;
	//This will set the expire tag, like in NGINX if you do location ~* \.(js|css|png|jpg|jpeg|gif|ico)$ { expires XX;
	uint staticFileCacheTTL = 0;

	std::string logFolder;

	int         worker  = 1;
	std::string address = "127.0.0.1";
	ushort      port    = 8081;
};

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
