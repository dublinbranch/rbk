#ifndef RBK_HTTP_CLIENTIP_H
#define RBK_HTTP_CLIENTIP_H

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class BeastConf;

/**
 * Who is the client of a request that arrived through a reverse proxy?
 *
 * The answer usually ends up in a DB filter, a rate limit key or a log line, so it must never
 * be a value the client can choose, and it must never be text that only looks like an address.
 * Both mistakes were live in digitalSpine until 18 Aug 2026: the first element of
 * X-Forwarded-For went unescaped into a query. See docs/clientIp.md for the whole story,
 * including why this uses Boost.Asio and not QHostAddress.
 *
 * Rules:
 *  - Only a trusted peer may tell us who the client is (isTrustedProxy).
 *  - With nginx $proxy_add_x_forwarded_for the RIGHTMOST element is the one nginx observed,
 *    everything to the left of it came from the client (rightmostForwardedFor).
 *  - Nothing that fails a strict address parse is passed on (parseAddress).
 *  - The value is the normalized spelling (normalizeAddress): v4-mapped IPv6 folds to
 *    dotted IPv4, so a dual-stack peer and a proxy header for the same host share one key.
 */
namespace rbk::Http {

/**
 * Strict address parse. Returns the canonical text form of an IPv4 or IPv6 address, or
 * nullopt. Deliberately rejects everything inet_aton would accept: "2130706433",
 * "0x7f.0.0.1", "1.2.3", "127.000.000.001", and leading or trailing whitespace.
 */
std::optional<std::string> parseAddress(std::string_view text);

/**
 * parseAddress, then fold a v4-mapped IPv6 address to dotted IPv4. Use this on anything
 * that becomes a stored value or a lookup key, so one address has one spelling. clientIp()
 * already returns this form.
 */
std::optional<std::string> normalizeAddress(std::string_view text);

/**
 * Rightmost comma separated element of an X-Forwarded-For style header, trimmed of spaces
 * and tabs and then normalized. X-Real-IP holds a single value, so rightmost == only.
 * nullopt when the header is empty or the last element is not an address.
 */
std::optional<std::string> rightmostForwardedFor(std::string_view headerValue);

/**
 * May this peer tell us who the client is? Loopback always may, which covers the usual
 * "nginx on the same host" setup. Any other peer must be listed in trustedProxies, as an
 * exact address (no CIDR, see docs/clientIp.md). Unparsable entries are ignored, they never
 * widen the match.
 */
bool isTrustedProxy(const boost::asio::ip::address&       peer,
                    const std::optional<std::vector<std::string>>& trustedProxies);

/**
 * The client address of one request: the socket peer, unless the peer is trusted and handed
 * us a usable X-Forwarded-For or X-Real-IP. Always a valid address in the normalizeAddress
 * spelling, never client controlled text. Falls back to "127.0.0.1" when the socket has no
 * peer (already closed).
 */
std::string clientIp(const boost::asio::ip::tcp::socket&                                   socket,
                     const boost::beast::http::request<boost::beast::http::string_body>&   req,
                     const std::optional<std::vector<std::string>>&                        trustedProxies);

/// Same, reading trustedProxies off the config.
std::string clientIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const BeastConf&                                                    conf);

/**
 * Address this request was aimed at. The socket local endpoint when the peer is untrusted;
 * X-Server-IP from a trusted peer (nginx $server_addr: the listen address the client used
 * to reach nginx). That is what getBasePath() needs when the process is bound on more than
 * one IP and the backend socket is only 127.0.0.1. Same parse rule as clientIp: an address
 * or we keep the local endpoint, never raw header text.
 */
std::string serverIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const std::optional<std::vector<std::string>>&                      trustedProxies);

std::string serverIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const BeastConf&                                                    conf);

} // namespace rbk::Http

#endif // RBK_HTTP_CLIENTIP_H
