#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace snipcli::common {

/// Whitespace characters stripped by trim().
inline constexpr std::string_view kWhitespace = " \t\r\n";

/**
 * @brief Returns an ASCII-lowercased copy of the input (locale-independent per byte).
 */
[[nodiscard]] std::string toLower(std::string_view input);

/**
 * @brief Returns the input with leading and trailing whitespace (space, \\t, \\r, \\n) removed.
 */
[[nodiscard]] std::string_view trim(std::string_view input) noexcept;

/**
 * @brief Compares two strings for equality ignoring ASCII case.
 */
[[nodiscard]] bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept;

/**
 * @brief Checks whether a string looks like a signed integer: optional '+'/'-' followed by
 * one or more ASCII digits. Used to decide whether a positional argument is meant as an ID.
 */
[[nodiscard]] bool looksNumeric(std::string_view s) noexcept;

/**
 * @brief Parses a strictly positive base-10 snippet ID with no sign, whitespace or suffix.
 * @return The ID, or std::nullopt if the text is empty, malformed, zero, or overflows uint64.
 */
[[nodiscard]] std::optional<uint64_t> parseStrictId(std::string_view s) noexcept;

}  // namespace snipcli::common
