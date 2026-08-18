#include "clientIp.h"

#include "beastConfig.h"

namespace rbk::Http {

namespace {

std::string_view trim(std::string_view value) {
	while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
		value.remove_prefix(1);
	}
	while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
		value.remove_suffix(1);
	}
	return value;
}

// One spelling for a key: v4-mapped IPv6 (::ffff:a.b.c.d, any case or hex form Asio
// accepts) becomes dotted IPv4. Dual-stack sockets report IPv4 peers this way;
// nginx $remote_addr does not. Without the fold those are two rate-limit keys
// and two access log files for the same host.
std::string addressToKey(const boost::asio::ip::address& addr) {
	if (addr.is_v6()) {
		const auto v6 = addr.to_v6();
		if (v6.is_v4_mapped()) {
			return boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped, v6).to_string();
		}
	}
	return addr.to_string();
}

} // namespace

std::optional<std::string> parseAddress(std::string_view text) {
	if (text.empty()) {
		return std::nullopt;
	}
	boost::system::error_code ec;
	const auto                addr = boost::asio::ip::make_address(std::string(text), ec);
	if (ec) {
		return std::nullopt;
	}
	return addr.to_string();
}

std::optional<std::string> normalizeAddress(std::string_view text) {
	if (text.empty()) {
		return std::nullopt;
	}
	boost::system::error_code ec;
	const auto                addr = boost::asio::ip::make_address(std::string(text), ec);
	if (ec) {
		return std::nullopt;
	}
	return addressToKey(addr);
}

std::optional<std::string> rightmostForwardedFor(std::string_view headerValue) {
	const auto comma = headerValue.rfind(',');
	if (comma != std::string_view::npos) {
		headerValue = headerValue.substr(comma + 1);
	}
	return normalizeAddress(trim(headerValue));
}

bool isTrustedProxy(const boost::asio::ip::address&                peer,
                    const std::optional<std::vector<std::string>>& trustedProxies) {
	if (peer.is_loopback()) {
		return true;
	}
	if (!trustedProxies) {
		return false;
	}
	for (const auto& entry : *trustedProxies) {
		boost::system::error_code ec;
		const auto                addr = boost::asio::ip::make_address(entry, ec);
		if (!ec && addr == peer) {
			return true;
		}
	}
	return false;
}

std::string clientIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const std::optional<std::vector<std::string>>&                      trustedProxies) {
	boost::system::error_code ec;
	const auto                endpoint = socket.remote_endpoint(ec);
	if (ec) {
		return "127.0.0.1";
	}
	const auto peer = endpoint.address();

	if (isTrustedProxy(peer, trustedProxies)) {
		if (auto it = req.find("X-Forwarded-For"); it != req.end()) {
			if (const auto ip = rightmostForwardedFor(it->value()); ip) {
				return *ip;
			}
		}
		if (auto it = req.find("X-Real-IP"); it != req.end()) {
			if (const auto ip = rightmostForwardedFor(it->value()); ip) {
				return *ip;
			}
		}
	}

	return addressToKey(peer);
}

std::string clientIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const BeastConf&                                                    conf) {
	return clientIp(socket, req, conf.trustedProxies);
}

std::string serverIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const std::optional<std::vector<std::string>>&                      trustedProxies) {
	boost::system::error_code ec;
	const auto                local = socket.local_endpoint(ec);
	const auto                bound = ec ? std::string("127.0.0.1") : local.address().to_string();

	const auto peer = socket.remote_endpoint(ec);
	if (ec || !isTrustedProxy(peer.address(), trustedProxies)) {
		return bound;
	}
	if (auto it = req.find("X-Server-IP"); it != req.end()) {
		if (const auto ip = parseAddress(trim(it->value())); ip) {
			return *ip;
		}
	}
	return bound;
}

std::string serverIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const BeastConf&                                                    conf) {
	return serverIp(socket, req, conf.trustedProxies);
}

} // namespace rbk::Http
