#pragma once

#include <algorithm>
#include <memory>
#include <rbk/mapExtensor/mapV2.h>
#include <string_view>

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
	/* Runs after the answer of immediate() is written, so the client does not wait for it (flush a few sql,
	 * logging). It runs on the same worker thread: keep it short, the other connections of this worker wait.
	 * Long work (a backup) goes to its own thread. Other requests can run on this thread between immediate()
	 * and deferred(): use only the members of this object, never status, payload or the thread local request
	 * state (dk()). Not called when immediate() throws. */
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

	// A RequestBase route (its deferred() can be empty): the HTTP session runs deferred() once the answer is written.
	[[nodiscard]] bool hasDeferred() const {
		return operation != nullptr;
	}

      private:
	std::shared_ptr<RequestBase> operation = nullptr;
};

mapV2<std::string, RequestBase*>     getDefaultRouting();
mapV2<std::string, SimpleRoutedType> getDefaultSimpleRouting();

// Unguessable ops URLs. Status is threads / cache / host / network, its TCP socket page is this
// token + "/sockets". Jemalloc stats is this token + "/stats".
inline constexpr char statusPagePath[]    = "Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww";
inline constexpr char statusSocketsPath[] = "Z4DgMzxU1gKlwhedSGeERZVeId4QRwDHDejwn3PKRQhdVLrzCg2ww/sockets";
inline constexpr char jemallocPagePath[]  = "ziAvks8rGUyQcoMU5K3eC9QsUvYv0lnK5iv7d8wdBWb7WAoa";
inline constexpr char jemallocStatsPath[] = "ziAvks8rGUyQcoMU5K3eC9QsUvYv0lnK5iv7d8wdBWb7WAoa/stats";

// The text holds one of the two tokens above, or a near miss of one: anywhere (a raw target, a
// query, "/Z4Dg…/sockets/", a doubled slash), in any case, cut short, or with a typo. The first or
// the last 16 characters of a token are enough (about 95 bits each: no real path matches by chance).
// Their path is the secret: a log must not keep it, nor the 400 answer that repeats it.
[[nodiscard]] inline bool hasSecretRbkToken(std::string_view text) {
	constexpr size_t kPart = 16;
	auto icontains         = [text](std::string_view part) {
        auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
        auto it    = std::search(text.begin(), text.end(), part.begin(), part.end(),
                                 [&](char a, char b) { return lower(a) == lower(b); });
        return it != text.end();
	};
	for (const std::string_view token : {std::string_view(statusPagePath), std::string_view(jemallocPagePath)}) {
		if (icontains(token.substr(0, kPart)) || icontains(token.substr(token.size() - kPart))) {
			return true;
		}
	}
	return false;
}
