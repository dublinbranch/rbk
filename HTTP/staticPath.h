#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

/**
 * Map a URL path (no leading slash) onto staticRoot.
 *
 * Returns nullopt if the path would leave the tree: `..`, an absolute path
 * (`/etc/passwd` after `//etc/passwd`), NUL, or a backslash. Callers must not
 * join staticRoot / urlPath themselves — `path / "/etc/passwd"` replaces the root.
 */
inline std::optional<std::filesystem::path> resolveStaticFile(const std::filesystem::path& staticRoot,
                                                             std::string_view             urlPath) {
	if (urlPath.empty() || urlPath.front() == '/' || urlPath.find('\0') != std::string_view::npos ||
	    urlPath.find('\\') != std::string_view::npos) {
		return std::nullopt;
	}

	const std::filesystem::path rel{std::string(urlPath)};
	if (rel.empty() || rel.is_absolute()) {
		return std::nullopt;
	}
	for (const auto& part : rel) {
		if (part == "..") {
			return std::nullopt;
		}
	}

	const auto root = staticRoot.lexically_normal();
	if (root.empty()) {
		return std::nullopt;
	}
	const auto out = (root / rel).lexically_normal();

	auto rootPrefix = root.generic_string();
	if (rootPrefix.back() != '/') {
		rootPrefix += '/';
	}
	const auto outStr = out.generic_string();
	if (outStr.size() <= rootPrefix.size() || outStr.compare(0, rootPrefix.size(), rootPrefix) != 0) {
		return std::nullopt;
	}
	return out;
}
