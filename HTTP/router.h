#pragma once

#include <memory>
#include <rbk/mapExtensor/mapV2.h>

class PMFCGI;
class Payload;
class RequestBase {
      public:
	//factory
	/*
	 * std::shared_ptr<RequestBase> CreateContainerFromJson::create() {
	return std::make_shared<CreateContainerFromJson>();
}
	 */
	[[nodiscard]] virtual std::shared_ptr<RequestBase> create() const = 0;
	//You are required to do something
	virtual void immediate(PMFCGI& status, Payload& payload) = 0;
	//You are not required to do something after
	virtual void deferred() {};
	//https://stackoverflow.com/questions/10024796/c-virtual-functions-but-no-virtual-destructors
	virtual ~RequestBase() = default;
};

using SimpleRoutedType = void (*)(PMFCGI& status, Payload& payload);

class BeastConf;
class Router {
      public:
	//Il principio è meno cose da fare meno errori possono esserci
	//Tutte le cose STRETTAMENTE NECESSARIE a mostrare il risultato in pagina
	void immediate(PMFCGI& status, const BeastConf*, Payload& payload);

	//Tutto il resto che magari può dare errori o eccezioni
	void deferred();

      private:
	std::shared_ptr<RequestBase> operation = nullptr;
};

mapV2<std::string, RequestBase*>     getDefaultRouting();
mapV2<std::string, SimpleRoutedType> getDefaultSimpleRouting();
