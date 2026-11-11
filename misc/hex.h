#ifndef RBK_MISC_HEX_H
#define RBK_MISC_HEX_H

// Bytes <-> hex text, two digits per byte.
//
// The case is the caller's choice, and it matters when the text is stored or compared: pick
// upper() or lower() for a value and keep it. decode() reads either case.
// No "0x" prefix and no separators either way: a caller that accepts them strips them first.

#include "rbk/number/intTypes.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rbk::Hex {

/** The value of one hex digit of either case, or -1 for any other character. */
[[nodiscard]] constexpr int digit(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

/** Upper case: {0x0a, 0xff} -> "0AFF". */
[[nodiscard]] std::string upper(std::span<const u8> bytes);

/** Lower case: {0x0a, 0xff} -> "0aff". */
[[nodiscard]] std::string lower(std::span<const u8> bytes);

/** The bytes of a string (a raw frame, a digest), lower case. */
[[nodiscard]] std::string lower(std::string_view bytes);

/** Exactly out.size() bytes from hex digits of either case.
 *  False when hex.size() != out.size() * 2 or a character is not a hex digit; out is then
 *  partly written, so a caller that keeps it must reset it. */
[[nodiscard]] bool decode(std::string_view hex, std::span<u8> out);

/** As many bytes as the text holds, from hex digits of either case.
 *  nullopt on an odd length or a character that is not a hex digit; empty text gives an empty
 *  vector. */
[[nodiscard]] std::optional<std::vector<u8>> decode(std::string_view hex);

} // namespace rbk::Hex

#endif // RBK_MISC_HEX_H
