#include "snipcli/common/string_utils.hpp"

#include <cctype>
#include <charconv>

namespace snipcli::common {

namespace {

char lowerAscii(char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool isDigit(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

}  // namespace

std::string toLower(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        out.push_back(lowerAscii(c));
    }
    return out;
}

std::string_view trim(std::string_view input) noexcept {
    const size_t first = input.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const size_t last = input.find_last_not_of(kWhitespace);
    return input.substr(first, last - first + 1);
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (lowerAscii(a[i]) != lowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

bool looksNumeric(std::string_view s) noexcept {
    if (!s.empty() && (s.front() == '+' || s.front() == '-')) {
        s.remove_prefix(1);
    }
    if (s.empty()) {
        return false;
    }
    for (char c : s) {
        if (!isDigit(c)) {
            return false;
        }
    }
    return true;
}

std::optional<uint64_t> parseStrictId(std::string_view s) noexcept {
    if (s.empty()) {
        return std::nullopt;
    }
    uint64_t id = 0;
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), id);
    if (ec != std::errc{} || end != s.data() + s.size() || id == 0) {
        return std::nullopt;
    }
    return id;
}

}  // namespace snipcli::common
