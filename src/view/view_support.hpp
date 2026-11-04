#pragma once

// Internal helpers shared by concrete views. Not part of the public include/ API.

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

namespace snipcli::view::detail {

/**
 * @brief Writes content byte-for-byte and flushes.
 *
 * On Windows, stdout is temporarily switched to binary mode so that "\n" is not expanded to
 * "\r\n", keeping `snip get --code-only > file` byte-exact on every platform.
 */
inline void writeRaw(std::ostream& out, const std::string& content) {
#if defined(_WIN32)
    if (&out == &std::cout) {
        out.flush();
        const int prevMode = _setmode(_fileno(stdout), _O_BINARY);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
        if (prevMode != -1) {
            _setmode(_fileno(stdout), prevMode);
        }
        return;
    }
#endif
    out << content;
    out.flush();
}

/**
 * @brief Joins items as `<prefix>item<suffix>` separated by @p separator.
 */
inline std::string join(const std::vector<std::string>& items, std::string_view separator,
                        std::string_view prefix = {}, std::string_view suffix = {}) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            out += separator;
        }
        out += prefix;
        out += items[i];
        out += suffix;
    }
    return out;
}

}  // namespace snipcli::view::detail
