#include "canned_reply.h"

namespace {

constexpr std::string_view pick(bool keepAlive, std::string_view ka, std::string_view close) {
	return keepAlive ? ka : close;
}

} // namespace

std::string_view cannedEmptyHttp(unsigned statusCode, unsigned httpVersion, bool keepAlive) noexcept {
	switch (statusCode) {
	case 204:
		if (httpVersion == 11) {
			return pick(keepAlive,
			            "HTTP/1.1 204 No Content\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
		}
		if (httpVersion == 10) {
			return pick(keepAlive,
			            "HTTP/1.0 204 No Content\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.0 204 No Content\r\nConnection: close\r\n\r\n");
		}
		break;
	case 205:
		if (httpVersion == 11) {
			return pick(keepAlive,
			            "HTTP/1.1 205 Reset Content\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.1 205 Reset Content\r\nConnection: close\r\n\r\n");
		}
		if (httpVersion == 10) {
			return pick(keepAlive,
			            "HTTP/1.0 205 Reset Content\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.0 205 Reset Content\r\nConnection: close\r\n\r\n");
		}
		break;
	case 304:
		if (httpVersion == 11) {
			return pick(keepAlive,
			            "HTTP/1.1 304 Not Modified\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.1 304 Not Modified\r\nConnection: close\r\n\r\n");
		}
		if (httpVersion == 10) {
			return pick(keepAlive,
			            "HTTP/1.0 304 Not Modified\r\nConnection: keep-alive\r\n\r\n",
			            "HTTP/1.0 304 Not Modified\r\nConnection: close\r\n\r\n");
		}
		break;
	default:
		break;
	}
	return {};
}
