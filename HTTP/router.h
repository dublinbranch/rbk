#pragma once

#include "rbk/HTTP/Payload.h"
#include <memory>
#include <rbk/mapExtensor/mapV2.h>

struct PMFCGI;
class RequestBase {
      public:
	//factory
	virtual std::shared_ptr<RequestBase> create() const = 0;
	//You are required to do something
	virtual Payload immediate(PMFCGI& status) = 0;
	//You are not required to do something after
	virtual void deferred(){};
	//https://stackoverflow.com/questions/10024796/c-virtual-functions-but-no-virtual-destructors
	/**
	 * #include <iostream>

class Base {};
class Derived: public Base { public: ~Derived() { std::cout << "Aargh\n"; } };

int main() {
  Base* b = new Derived();
  Derived* d = new Derived();

  delete d;
  delete b;
}

This prints:

Aargh

Yep, only once. Now I hardly believe this will ever happen in our case to rely on the dtor
but the note is here
*/
	virtual ~RequestBase() = default;
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

mapV2<std::string, RequestBase*> getDefaultRouting();