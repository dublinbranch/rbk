#ifndef HOME_ROY_PUBLIC_RBK_HTTP_BEASTWEBSOCKETUPGRADE_H
#define HOME_ROY_PUBLIC_RBK_HTTP_BEASTWEBSOCKETUPGRADE_H

// Heavy header: include only from TUs that set a websocket upgrade hook.
#include "rbk/HTTP/beastConfig.h"

#include <boost/beast/http.hpp>
#include <utility>

/* Same as assigning conf.websocketUpgrade directly; kept for callers that prefer a setter.
 * The socket keeps its worker executor (rbk::Http::WorkerSocket), see beastConfig.h.
 * Return contract: true = the hook moved from the socket and owns it; false = the hook did
 * not touch either argument and the request goes to normal routing.
 */
inline void setWebsocketUpgrade(BeastConf& conf, BeastConf::WebSocketUpgradeFn fn) {
	conf.websocketUpgrade = std::move(fn);
}

#endif
