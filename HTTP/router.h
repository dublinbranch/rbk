#ifndef ROUTER_H
#define ROUTER_H

#include "PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include <memory>

struct PMFCGI;
class RequestBase {
      public:
	//factory
	virtual std::shared_ptr<RequestBase> create() const = 0;
	//You are required to do something
	virtual Payload immediate(PMFCGI& status) = 0;
	//You are not required to do something after
	virtual void deferred(){};
};

class Router {
      public:
	//Il principio è meno cose da fare meno errori possono esserci
	//Tutte le cose STRETTAMENTE NECESSARIE a mostrare il risultato in pagina
	Payload immediate(PMFCGI& status);

	//Tutto il resto che magari può dare errori o eccezioni
	void deferred();

      private:
	std::shared_ptr<RequestBase> operation = nullptr;
};

mapV2<std::string, RequestBase *> getDefaultRouting();
#endif // ROUTER_H
