#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H

#include "rbk/mapExtensor/mapV2.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>
class RequestBase;

/* Thread-per-core: each HttpHandler thread drives its own single threaded io_context, so a
 * connection is only ever touched by the thread that accepted it and no strand is needed.
 * That also lets the executor stay a concrete type instead of asio's type erased
 * any_io_executor, which was copied, moved and destroyed on every handler dispatch.
 *
 * These aliases are public because the websocket upgrade hook hands the socket across, and
 * converting it to a plain tcp::socket there would erase the executor again - which is
 * exactly what it used to do.
 */
namespace rbk::Http {
using WorkerExecutor = boost::asio::io_context::executor_type;
using WorkerSocket   = boost::asio::basic_stream_socket<boost::asio::ip::tcp, WorkerExecutor>;
} // namespace rbk::Http

class PMFCGI;
class Payload;

class BeastConf {
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

	/* Called for every websocket upgrade request, before prePhase1 / loginManager.
	 * Return true  = the hook took the socket (moved from it) and owns the connection.
	 * Return false = not handled. The hook MUST NOT have moved from the socket or the request;
	 *                both go on to normal routing, so an unknown path gets a normal 404.
	 * Take both parameters as rvalue references (not by value), or the move happens at the
	 * call and false can no longer be honoured.
	 */
	using WebSocketUpgradeFn = std::function<bool(
	    rbk::Http::WorkerSocket&& socket,
	    boost::beast::http::request<boost::beast::http::string_body>&& req)>;

	WebSocketUpgradeFn websocketUpgrade;

	// Runs on each HttpHandler thread before it starts draining the io_context.
	// digitalSpine uses this to turn CLIENT_MULTI_STATEMENTS off on the worker
	// connection (SQLBuffering stays on its own threads).
	std::function<void()> onWorkerStart;

	mapV2<std::string, SimpleRoutedType> routingSimple;
	mapV2<std::string, RequestBase*>     routing;
	mapV2<std::string, std::string>      defaultHeader;

	//if SET in case a path IS NOT MAPPED it will be searched on DISK inside this PROGRAM RELATIVE folder
	std::optional<std::filesystem::path> staticFile;
	//This will set the expire tag, like in NGINX if you do location ~* \.(js|css|png|jpg|jpeg|gif|ico)$ { expires XX;
	uint staticFileCacheTTL = 0;

	std::string logFolder = "httpLog";

	uint        worker  = 1;
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
	std::optional<std::vector<std::string>> logWhitelist;
	std::optional<std::vector<std::string>> logBlacklist;
	void                                    setBasePath(const std::string& newBasePath);

	/**
	 * @brief basePath is used to set the default path from where to read content, sometimes, in certain case we listen on multiple ip etc
	 */
	std::optional<std::string> basePath;

	/**
	 * @brief trustedProxies are the peers whose X-Forwarded-For / X-Real-IP we believe.
	 * A request from anything else is taken at face value: the socket peer is the client and
	 * the headers are ignored. Loopback is always trusted, so nginx on the same host needs no
	 * entry here. Exact addresses only, no CIDR. Used by rbk::Http::clientIp, see
	 * HTTP/docs/clientIp.md.
	 * Optional so config files written before this key keep loading.
	 */
	std::optional<std::vector<std::string>> trustedProxies;

      private:
};

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_HTTP_BEASTCONFIG_H
