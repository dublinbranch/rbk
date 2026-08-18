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
	if (text.starts_with("::ffff:")) {
		text.remove_prefix(7);
	}
	return parseAddress(text);
}

std::optional<std::string> rightmostForwardedFor(std::string_view headerValue) {
	const auto comma = headerValue.rfind(',');
	if (comma != std::string_view::npos) {
		headerValue = headerValue.substr(comma + 1);
	}
	return parseAddress(trim(headerValue));
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

	return peer.to_string();
}

std::string clientIp(const boost::asio::ip::tcp::socket&                                 socket,
                     const boost::beast::http::request<boost::beast::http::string_body>& req,
                     const BeastConf&                                                    conf) {
	return clientIp(socket, req, conf.trustedProxies);
}

} // namespace rbk::Http
