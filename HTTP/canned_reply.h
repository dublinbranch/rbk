#ifndef RBK_HTTP_CANNED_REPLY_H
#define RBK_HTTP_CANNED_REPLY_H

#include <string_view>

// Pre-serialized 204 / 205 / 304 with only Connection. No Content-Type, no body.
// Empty if this status/version is not a canned empty reply (use the Beast serializer).
// httpVersion is Beast's: 10 or 11.
std::string_view cannedEmptyHttp(unsigned statusCode, unsigned httpVersion, bool keepAlive) noexcept;

#endif
