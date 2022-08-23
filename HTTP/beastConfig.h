#ifndef BEASTCONFIG_H
#define BEASTCONFIG_H

#include "rbk/mapExtensor/mapV2.h"
class RequestBase;

struct BeastConf {
	mapV2<std::string, RequestBase*> routing;
	mapV2<std::string, std::string>  defaultHeader;

	std::string configFolder;

	int         worker  = 1;
	std::string address = "127.0.0.1";
	ushort      port    = 8081;
};

#endif // BEASTCONFIG_H
