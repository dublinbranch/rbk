#ifndef HOME_ROY_PUBLIC_RBK_HTTP_BEASTWEBSOCKETUPGRADE_H
#define HOME_ROY_PUBLIC_RBK_HTTP_BEASTWEBSOCKETUPGRADE_H

// Heavy header: include only from TUs that set a websocket upgrade hook.
#include "rbk/HTTP/beastConfig.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <utility>

inline void setWebsocketUpgrade(
    BeastConf& conf,
    std::function<bool(
        boost::asio::ip::tcp::socket&&,
        boost::beast::http::request<boost::beast::http::string_body>&&)> fn) {
	conf.websocketUpgrade = [f = std::move(fn)](void* socket, void* request) -> bool {
		using tcp     = boost::asio::ip::tcp;
		using request = boost::beast::http::request<boost::beast::http::string_body>;
		return f(std::move(*static_cast<tcp::socket*>(socket)),
		         std::move(*static_cast<request*>(request)));
	};
}

#endif
