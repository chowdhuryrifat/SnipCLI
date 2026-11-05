#include "snipcli/model/json_snippet_repository.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#endif

#include "snipcli/common/exceptions.hpp"
#include "snipcli/common/string_utils.hpp"

namespace snipcli::model {

namespace {

std::filesystem::path canonicalizeStorageTarget(const std::filesystem::path& p) {
    constexpr int kMaxSymlinkHops = 40;
    std::filesystem::path current = p;
    int hops = 0;
    std::error_code ec;

    while (std::filesystem::is_symlink(current, ec)) {
        if (++hops > kMaxSymlinkHops) {
            throw common::StorageException(
                "Too many levels of symbolic links while resolving storage path: " + p.string());
        }
        auto linkDest = std::filesystem::read_symlink(current, ec);
        if (ec) {
            break;
        }
        std::filesystem::path parent =
            current.has_parent_path() ? current.parent_path() : std::filesystem::current_path();
        if (linkDest.is_relative()) {
            linkDest = parent / linkDest;
        }
        current = linkDest;
    }
    ec.clear();

    if (std::filesystem::exists(current, ec)) {
        auto canon = std::filesystem::canonical(current, ec);
        if (!ec) {
            return canon;
        }
    }
    ec.clear();

    std::filesystem::path parent =
        current.has_parent_path() ? current.parent_path() : std::filesystem::current_path();
    std::filesystem::create_directories(parent, ec);
    ec.clear();

    auto canonParent = std::filesystem::canonical(parent, ec);
    if (!ec) {
        std::filesystem::path resolved = canonParent / current.filename();
        if (std::filesystem::exists(resolved, ec)) {
            auto canonResolved = std::filesystem::canonical(resolved, ec);
            if (!ec) {
                return canonResolved;
            }
        }
        return resolved;
    }

    ec.clear();
    auto weakly = std::filesystem::weakly_canonical(current, ec);
    if (!ec) {
        return weakly;
    }

    return current;
}

class FileLock {
public:
    /// @param target The already-canonicalized storage file; the lock lives at <target>.lock.
    explicit FileLock(const std::filesystem::path& target) {
        std::error_code ec;
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path(), ec);
            if (ec) {
                throw common::StorageException("Unable to create storage directory for lock: " +
                                               target.parent_path().string());
            }
        }
        std::filesystem::path lockPath = target;
        lockPath += ".lock";

        int timeoutMs =
            JsonSnippetRepository::parseLockTimeoutMs(std::getenv("SNIPCLI_LOCK_TIMEOUT_MS"));

#if defined(_WIN32)
        // Windows file locking via CreateFileW/LockFileEx operates directly on filesystem handles.
        // On Windows with Developer Mode, symlinks can be created; ensure we reject them
        // (O_NOFOLLOW equivalent).
        std::error_code symlinkEc;
        auto st = std::filesystem::symlink_status(lockPath, symlinkEc);
        if (!symlinkEc && std::filesystem::is_symlink(st)) {
            throw common::StorageException("Lock path is a symbolic link; refusing to follow: " +
                                           lockPath.string());
        }
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(lockPath.wstring().c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            bool isReparse = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            FindClose(hFind);
            if (isReparse) {
                throw common::StorageException(
                    "Lock path is a symbolic link; refusing to follow: " + lockPath.string());
            }
        }

        std::wstring wPath = lockPath.wstring();
        m_handle = CreateFileW(wPath.c_str(), GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            throw common::StorageException("Failed to open lock file: " + lockPath.string());
        }
        auto start = std::chrono::steady_clock::now();
        bool acquired = false;
        while (true) {
            OVERLAPPED ov{};
            if (LockFileEx(m_handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                           MAXDWORD, MAXDWORD, &ov)) {
                acquired = true;
                break;
            }
            DWORD err = GetLastError();
            if (err != ERROR_LOCK_VIOLATION && err != ERROR_SHARING_VIOLATION) {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
                throw common::StorageException("Failed to acquire lock on Windows: " +
                                               lockPath.string());
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            if (elapsed >= timeoutMs) {
                break;
            }
            Sleep(50);
        }
        if (!acquired) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            throw common::StorageException("Storage is locked by another snip process (" +
                                           lockPath.string() + ").");
        }
#else
        m_fd = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (m_fd < 0) {
            int err = errno;
            if (err == ELOOP
#if defined(EMLINK)
                || err == EMLINK
#endif
            ) {
                throw common::StorageException(
                    "Lock path is a symbolic link; refusing to follow: " + lockPath.string());
            }
            throw common::StorageException("Failed to open lock file: " + lockPath.string() + ": " +
                                           std::strerror(err));
        }
        auto start = std::chrono::steady_clock::now();
        bool acquired = false;
        while (true) {
            if (::flock(m_fd, LOCK_EX | LOCK_NB) == 0) {
                acquired = true;
                break;
            }
            int err = errno;
            if (err != EWOULDBLOCK && err != EAGAIN && err != EINTR) {
                ::close(m_fd);
                m_fd = -1;
                throw common::StorageException("Failed to acquire flock on " + lockPath.string() +
                                               ": " + std::strerror(err));
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            if (elapsed >= timeoutMs) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!acquired) {
            ::close(m_fd);
            m_fd = -1;
            throw common::StorageException("Storage is locked by another snip process (" +
                                           lockPath.string() + ").");
        }
#endif
    }

    ~FileLock() {
#if defined(_WIN32)
        if (m_handle != INVALID_HANDLE_VALUE) {
            OVERLAPPED ov{};
            UnlockFileEx(m_handle, 0, MAXDWORD, MAXDWORD, &ov);
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
#else
        if (m_fd >= 0) {
            ::flock(m_fd, LOCK_UN);
            ::close(m_fd);
            m_fd = -1;
        }
#endif
    }

    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

private:
#if defined(_WIN32)
    HANDLE m_handle{INVALID_HANDLE_VALUE};
#else
    int m_fd{-1};
#endif
};

/**
 * Returns the current time as an ISO 8601 timestamp in the user's local timezone with an
 * explicit UTC offset (e.g. "2026-11-04T22:17:18+06:00"). The offset keeps the instant
 * unambiguous while showing the wall-clock time the user actually experienced.
 */
std::string getIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    std::tm utc{};
#if defined(_WIN32)
    localtime_s(&local, &now_c);
    gmtime_s(&utc, &now_c);
#else
    localtime_r(&now_c, &local);
    gmtime_r(&now_c, &utc);
#endif

    // Portable UTC offset: difference between the local and UTC broken-down times.
    // The two can differ by at most one calendar day (including across a year boundary).
    long dayDiff = 0;
    if (local.tm_year != utc.tm_year) {
        dayDiff = local.tm_year > utc.tm_year ? 1 : -1;
    } else {
        dayDiff = static_cast<long>(local.tm_yday) - static_cast<long>(utc.tm_yday);
    }
    long offsetMinutes =
        (dayDiff * 24 + (local.tm_hour - utc.tm_hour)) * 60 + (local.tm_min - utc.tm_min);

    const char sign = offsetMinutes < 0 ? '-' : '+';
    if (offsetMinutes < 0) {
        offsetMinutes = -offsetMinutes;
    }

    std::ostringstream ss;
    ss << std::put_time(&local, "%Y-%m-%dT%H:%M:%S") << sign << std::setw(2) << std::setfill('0')
       << (offsetMinutes / 60) << ':' << std::setw(2) << std::setfill('0') << (offsetMinutes % 60);
    return ss.str();
}

std::string escapeJson(std::string_view input) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() + input.size() / 8 + 2);
    for (char c : input) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default: {
                const auto uc = static_cast<unsigned char>(c);
                if (uc < 0x20) {
                    out += "\\u00";
                    out += kHex[uc >> 4];
                    out += kHex[uc & 0x0F];
                } else {
                    out += c;
                }
                break;
            }
        }
    }
    return out;
}

/**
 * Scans a JSON string body that starts at @p start (just after its opening quote) and returns
 * the index of the closing quote, which must occur before @p last.
 *
 * Escapes are skipped in a single forward pass (O(n)). This is the one shared scanner for
 * keys, values and tags. Escape *validity* is checked separately by unescapeJson.
 *
 * @throws common::StorageException with @p controlMsg on a raw control character, or with
 *         @p unterminatedMsg if the string is not closed before @p last.
 */
size_t findStringEnd(const std::string& s, size_t start, size_t last, const char* controlMsg,
                     const char* unterminatedMsg) {
    for (size_t i = start; i < last; ++i) {
        auto c = static_cast<unsigned char>(s[i]);
        if (c == '"') {
            return i;
        }
        if (c == '\\') {
            if (++i >= last) {
                break;
            }
            c = static_cast<unsigned char>(s[i]);
        }
        if (c < 0x20) {
            throw common::StorageException(controlMsg);
        }
    }
    throw common::StorageException(unterminatedMsg);
}

bool hex4(const std::string& s, size_t at, unsigned& out) {
    if (at + 4 > s.size()) {
        return false;
    }
    out = 0;
    for (size_t k = 0; k < 4; ++k) {
        unsigned char h = static_cast<unsigned char>(s[at + k]);
        if (!std::isxdigit(h)) {
            return false;
        }
        out = out * 16 + (h <= '9' ? h - '0' : (std::tolower(h) - 'a' + 10));
    }
    return true;
}

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

std::string unescapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\') {
            if (i + 1 >= input.size()) {
                throw common::StorageException("Dangling backslash escape at end of JSON string.");
            }
            char next = input[++i];
            switch (next) {
                case '"':
                    out += '"';
                    break;
                case '\\':
                    out += '\\';
                    break;
                case '/':
                    out += '/';
                    break;
                case 'b':
                    out += '\b';
                    break;
                case 'f':
                    out += '\f';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                case 'u': {
                    unsigned cp = 0;
                    if (!hex4(input, i + 1, cp)) {
                        throw common::StorageException(
                            "Invalid hexadecimal sequence in \\u escape.");
                    }
                    i += 4;
                    if (cp == 0) {
                        throw common::StorageException(
                            "Embedded NUL character '\\u0000' is not permitted in JSON string.");
                    }
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (i + 6 < input.size() && input[i + 1] == '\\' && input[i + 2] == 'u') {
                            unsigned lo = 0;
                            if (hex4(input, i + 3, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                i += 6;
                            } else {
                                throw common::StorageException(
                                    "High surrogate \\u without matching low surrogate.");
                            }
                        } else {
                            throw common::StorageException(
                                "Isolated high surrogate in Unicode escape.");
                        }
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        throw common::StorageException("Isolated low surrogate in Unicode escape.");
                    }
                    if (cp > 0x10FFFF) {
                        throw common::StorageException(
                            "Unicode code point exceeds 0x10FFFF maximum.");
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default:
                    throw common::StorageException(std::string("Invalid escape sequence '\\") +
                                                   next + "' in JSON string.");
            }
        } else {
            if (input[i] == '\0') {
                throw common::StorageException(
                    "Embedded NUL character is not permitted in JSON string.");
            }
            if (static_cast<unsigned char>(input[i]) < 0x20) {
                throw common::StorageException(
                    "Corrupt storage file: unescaped control character in JSON string.");
            }
            out += input[i];
        }
    }
    return out;
}

bool isValidUtf8(std::string_view s) noexcept {
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            i++;
        } else if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            if ((c1 & 0xC0) != 0x80)
                return false;
            i += 2;
        } else if (c == 0xE0) {
            if (i + 2 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            if (c1 < 0xA0 || c1 > 0xBF || (c2 & 0xC0) != 0x80)
                return false;
            i += 3;
        } else if ((c >= 0xE1 && c <= 0xEC) || c == 0xEE || c == 0xEF) {
            if (i + 2 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
                return false;
            i += 3;
        } else if (c == 0xED) {
            if (i + 2 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            if (c1 < 0x80 || c1 > 0x9F || (c2 & 0xC0) != 0x80)
                return false;
            i += 3;
        } else if (c == 0xF0) {
            if (i + 3 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            unsigned char c3 = static_cast<unsigned char>(s[i + 3]);
            if (c1 < 0x90 || c1 > 0xBF || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                return false;
            i += 4;
        } else if (c >= 0xF1 && c <= 0xF3) {
            if (i + 3 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            unsigned char c3 = static_cast<unsigned char>(s[i + 3]);
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                return false;
            i += 4;
        } else if (c == 0xF4) {
            if (i + 3 >= s.size())
                return false;
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
            unsigned char c3 = static_cast<unsigned char>(s[i + 3]);
            if (c1 < 0x80 || c1 > 0x8F || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

constexpr int kMaxJsonDepth = 64;

bool isValidJsonNumber(std::string_view s) noexcept {
    if (s.empty()) {
        return false;
    }
    size_t i = 0;
    if (s[i] == '-') {
        i++;
        if (i == s.size()) {
            return false;
        }
    }
    if (s[i] == '0') {
        i++;
    } else if (s[i] >= '1' && s[i] <= '9') {
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            i++;
        }
    } else {
        return false;
    }
    if (i < s.size() && s[i] == '.') {
        i++;
        if (i == s.size() || s[i] < '0' || s[i] > '9') {
            return false;
        }
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            i++;
        }
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
            i++;
        }
        if (i == s.size() || s[i] < '0' || s[i] > '9') {
            return false;
        }
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            i++;
        }
    }
    return i == s.size();
}

bool isValidJsonBareToken(std::string_view s) noexcept {
    if (s == "true" || s == "false" || s == "null") {
        return true;
    }
    return isValidJsonNumber(s);
}

void validateAndSkipJsonValue(const std::string& str, size_t& idx, size_t last, int depth) {
    while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
        idx++;
    }
    if (idx >= last) {
        throw common::StorageException("Corrupt storage file: expected JSON value.");
    }

    if (depth > kMaxJsonDepth) {
        throw common::StorageException(
            "Corrupt storage file: JSON nesting depth exceeds maximum supported (64).");
    }

    if (str[idx] == '"') {
        size_t valStart = idx + 1;
        const size_t valEnd =
            findStringEnd(str, valStart, last,
                          "Corrupt storage file: unescaped control character in JSON string.",
                          "Corrupt storage file: unterminated value string in JSON.");
        unescapeJson(str.substr(valStart, valEnd - valStart));
        idx = valEnd + 1;
    } else if (str[idx] == '[') {
        idx++;  // skip '['
        bool expectComma = false;
        while (idx < last) {
            while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
                idx++;
            }
            if (idx >= last) {
                break;
            }
            if (str[idx] == ']') {
                idx++;
                return;
            }
            if (expectComma) {
                if (str[idx] != ',') {
                    throw common::StorageException(
                        "Corrupt storage file: expected ',' in JSON array.");
                }
                idx++;
                while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
                    idx++;
                }
                if (idx >= last) {
                    throw common::StorageException(
                        "Corrupt storage file: unexpected end of array after comma.");
                }
                if (str[idx] == ']') {
                    throw common::StorageException(
                        "Corrupt storage file: trailing comma in JSON array.");
                }
            }
            validateAndSkipJsonValue(str, idx, last, depth + 1);
            expectComma = true;
        }
        throw common::StorageException("Corrupt storage file: unclosed array in JSON.");
    } else if (str[idx] == '{') {
        idx++;  // skip '{'
        bool expectComma = false;
        std::unordered_set<std::string> nestedKeys;
        while (idx < last) {
            while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
                idx++;
            }
            if (idx >= last) {
                break;
            }
            if (str[idx] == '}') {
                idx++;
                return;
            }
            if (expectComma) {
                if (str[idx] != ',') {
                    throw common::StorageException(
                        "Corrupt storage file: expected ',' in JSON object.");
                }
                idx++;
                while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
                    idx++;
                }
                if (idx >= last) {
                    throw common::StorageException(
                        "Corrupt storage file: unexpected end of object after comma.");
                }
                if (str[idx] == '}') {
                    throw common::StorageException(
                        "Corrupt storage file: trailing comma in JSON object.");
                }
            }
            if (str[idx] != '"') {
                throw common::StorageException(
                    "Corrupt storage file: object key must be a string literal.");
            }
            size_t keyStart = idx + 1;
            const size_t keyEnd =
                findStringEnd(str, keyStart, last,
                              "Corrupt storage file: unescaped control character in JSON key.",
                              "Corrupt storage file: unterminated key string in JSON object.");
            std::string nKey = unescapeJson(str.substr(keyStart, keyEnd - keyStart));
            if (!nestedKeys.insert(nKey).second) {
                throw common::StorageException("Corrupt storage file: duplicate key '" + nKey +
                                               "' in JSON object.");
            }
            idx = keyEnd + 1;

            while (idx < last && std::isspace(static_cast<unsigned char>(str[idx]))) {
                idx++;
            }
            if (idx >= last || str[idx] != ':') {
                throw common::StorageException(
                    "Corrupt storage file: expected ':' after key in JSON object.");
            }
            idx++;  // skip ':'

            validateAndSkipJsonValue(str, idx, last, depth + 1);
            expectComma = true;
        }
        throw common::StorageException("Corrupt storage file: unclosed nested object in JSON.");
    } else {
        size_t valStart = idx;
        while (idx < last && str[idx] != ',' && str[idx] != ']' && str[idx] != '}' &&
               !std::isspace(static_cast<unsigned char>(str[idx]))) {
            idx++;
        }
        std::string token = str.substr(valStart, idx - valStart);
        if (!isValidJsonBareToken(token)) {
            throw common::StorageException("Corrupt storage file: invalid JSON value '" + token +
                                           "'.");
        }
    }
}

std::unordered_map<std::string, std::string> parseJsonObject(const std::string& objStr) {
    std::unordered_map<std::string, std::string> fields;
    size_t first = objStr.find_first_not_of(" \t\n\r");
    size_t last = objStr.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || objStr[first] != '{' || last == std::string::npos ||
        objStr[last] != '}') {
        throw common::StorageException("Corrupt storage file: invalid JSON object format.");
    }

    size_t idx = first + 1;
    bool expectComma = false;

    while (idx < last) {
        while (idx < last && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
            idx++;
        }
        if (idx >= last) {
            break;
        }

        if (expectComma) {
            if (objStr[idx] != ',') {
                throw common::StorageException(
                    "Corrupt storage file: expected ',' between object fields.");
            }
            idx++;
            while (idx < last && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
                idx++;
            }
            if (idx >= last) {
                throw common::StorageException(
                    "Corrupt storage file: trailing comma in JSON object.");
            }
        }

        if (objStr[idx] != '"') {
            throw common::StorageException(
                "Corrupt storage file: object key must be a string literal.");
        }

        size_t keyStart = idx + 1;
        const size_t keyEnd =
            findStringEnd(objStr, keyStart, last,
                          "Corrupt storage file: unescaped control character in JSON key.",
                          "Corrupt storage file: unterminated key string in JSON object.");
        std::string key = unescapeJson(objStr.substr(keyStart, keyEnd - keyStart));
        idx = keyEnd + 1;

        while (idx < last && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
            idx++;
        }
        if (idx >= last || objStr[idx] != ':') {
            throw common::StorageException("Corrupt storage file: expected ':' after key '" + key +
                                           "'.");
        }
        idx++;  // skip ':'

        while (idx < last && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
            idx++;
        }
        if (idx >= last) {
            throw common::StorageException(
                "Corrupt storage file: expected value after ':' for key '" + key + "'.");
        }

        std::string value;
        if (objStr[idx] == '"') {
            size_t valStart = idx + 1;
            const size_t valEnd =
                findStringEnd(objStr, valStart, last,
                              "Corrupt storage file: unescaped control character in JSON string.",
                              "Corrupt storage file: unterminated value string in JSON object.");
            value = unescapeJson(objStr.substr(valStart, valEnd - valStart));
            idx = valEnd + 1;
        } else if (objStr[idx] == '[' || objStr[idx] == '{') {
            size_t subStart = idx;
            validateAndSkipJsonValue(objStr, idx, last, 2);
            value = objStr.substr(subStart, idx - subStart);
        } else {
            size_t valStart = idx;
            while (idx < last && objStr[idx] != ',' &&
                   !std::isspace(static_cast<unsigned char>(objStr[idx]))) {
                idx++;
            }
            std::string token = objStr.substr(valStart, idx - valStart);
            if (!isValidJsonBareToken(token)) {
                throw common::StorageException("Corrupt storage file: invalid JSON value '" +
                                               token + "'.");
            }
            value = std::move(token);
        }

        if (fields.count(key)) {
            throw common::StorageException("Corrupt storage file: duplicate key '" + key +
                                           "' in JSON object.");
        }
        fields[key] = std::move(value);
        expectComma = true;
    }

    return fields;
}

std::vector<std::string> parseJsonArray(const std::string& arrStr) {
    size_t first = arrStr.find_first_not_of(" \t\n\r");
    size_t last = arrStr.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || arrStr[first] != '[' || last == std::string::npos ||
        arrStr[last] != ']') {
        throw common::StorageException("Corrupt storage file: invalid tags array format.");
    }
    std::vector<std::string> elements;
    size_t i = first + 1;
    bool expectComma = false;
    while (i < last) {
        while (i < last && std::isspace(static_cast<unsigned char>(arrStr[i]))) {
            i++;
        }
        if (i >= last) {
            break;
        }
        if (expectComma) {
            if (arrStr[i] != ',') {
                throw common::StorageException("Corrupt storage file: expected ',' in tags array.");
            }
            i++;
            while (i < last && std::isspace(static_cast<unsigned char>(arrStr[i]))) {
                i++;
            }
            if (i >= last) {
                throw common::StorageException(
                    "Corrupt storage file: trailing comma in tags array.");
            }
        }
        if (arrStr[i] != '"') {
            throw common::StorageException(
                "Corrupt storage file: tags array elements must be string literals.");
        }
        const size_t end = findStringEnd(
            arrStr, i + 1, last, "Corrupt storage file: unescaped control character in tag string.",
            "Corrupt storage file: unterminated string in tags array.");
        elements.push_back(unescapeJson(arrStr.substr(i + 1, end - (i + 1))));
        i = end + 1;
        expectComma = true;
    }
    return elements;
}

/// Serializes snippets to the canonical on-disk JSON layout (2-space indent, one field per line).
std::string serializeSnippets(const std::vector<Snippet>& snippets) {
    std::string out;
    size_t estimate = 4;
    for (const auto& s : snippets) {
        estimate += 160 + s.getTitle().size() + s.getLanguage().size() + s.getCode().size() +
                    s.getCreatedAt().size() + s.getUpdatedAt().size();
        for (const auto& tag : s.getTags()) {
            estimate += tag.size() + 4;
        }
    }
    out.reserve(estimate + estimate / 8);

    auto appendStringField = [&out](const char* key, const std::string& value, bool last) {
        out += "    \"";
        out += key;
        out += "\": \"";
        out += escapeJson(value);
        out += last ? "\"\n" : "\",\n";
    };

    out += "[\n";
    for (size_t i = 0; i < snippets.size(); ++i) {
        const auto& s = snippets[i];
        out += "  {\n";
        out += "    \"id\": ";
        out += std::to_string(s.getId());
        out += ",\n";
        appendStringField("title", s.getTitle(), false);
        appendStringField("language", s.getLanguage(), false);
        appendStringField("code", s.getCode(), false);
        out += "    \"tags\": [";
        const auto& tags = s.getTags();
        for (size_t t = 0; t < tags.size(); ++t) {
            if (t > 0) {
                out += ", ";
            }
            out += '"';
            out += escapeJson(tags[t]);
            out += '"';
        }
        out += "],\n";
        appendStringField("createdAt", s.getCreatedAt(), false);
        appendStringField("updatedAt", s.getUpdatedAt(), true);
        out += (i + 1 < snippets.size()) ? "  },\n" : "  }\n";
    }
    out += "]\n";
    return out;
}

/**
 * Compacts snippet IDs to the contiguous sequence 1..N, preserving the existing relative
 * order (ascending by current ID). Called after every deletion so IDs never contain gaps.
 * Timestamps are deliberately left untouched: renumbering is not a content modification.
 */
void renumberSequentially(std::vector<Snippet>& snippets) {
    std::stable_sort(snippets.begin(), snippets.end(),
                     [](const Snippet& a, const Snippet& b) { return a.getId() < b.getId(); });
    uint64_t nextId = 1;
    for (auto& s : snippets) {
        s.setId(nextId++);
    }
}

}  // namespace

std::filesystem::path JsonSnippetRepository::getDefaultStoragePath() {
    const char* envPath = std::getenv("SNIPCLI_STORAGE_PATH");
    if (envPath && std::string(envPath).length() > 0) {
        return std::filesystem::path(envPath);
    }

#if defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData && std::string(appData).length() > 0) {
        return std::filesystem::path(appData) / "SnipCLI" / "snippets.json";
    }
#else
    const char* home = std::getenv("HOME");
    if (home && std::string(home).length() > 0) {
        return std::filesystem::path(home) / ".snipcli" / "snippets.json";
    }
#endif
    throw common::StorageException(
        "Cannot determine storage location. Please set SNIPCLI_STORAGE_PATH.");
}

int JsonSnippetRepository::parseLockTimeoutMs(const char* envVal) noexcept {
    constexpr int kDefaultTimeoutMs = 10000;
    constexpr int kMinTimeoutMs = 1;
    constexpr int kMaxTimeoutMs = 600000;

    if (!envVal || *envVal == '\0') {
        return kDefaultTimeoutMs;
    }
    std::string_view sv(envVal);
    if (sv.empty() || sv[0] == '-' || sv[0] == '+') {
        return kDefaultTimeoutMs;
    }
    int64_t val = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        return kDefaultTimeoutMs;
    }
    if (val <= 0) {
        return kDefaultTimeoutMs;
    }
    if (val < kMinTimeoutMs) {
        return kMinTimeoutMs;
    }
    if (val > kMaxTimeoutMs) {
        return kMaxTimeoutMs;
    }
    return static_cast<int>(val);
}

JsonSnippetRepository::JsonSnippetRepository() : JsonSnippetRepository(getDefaultStoragePath()) {}

JsonSnippetRepository::JsonSnippetRepository(std::filesystem::path storagePath)
    : m_storagePath(std::move(storagePath)) {}

const std::filesystem::path& JsonSnippetRepository::getStoragePath() const noexcept {
    return m_storagePath;
}

std::filesystem::path JsonSnippetRepository::resolveTarget() const {
    return canonicalizeStorageTarget(m_storagePath);
}

std::vector<Snippet> JsonSnippetRepository::loadFromFile(
    const std::filesystem::path& target) const {
    // Messages report the user-supplied path (not the resolved target) for recognisability.
    auto corrupt = [this](const std::string& detail) {
        return common::StorageException("Corrupt storage file at " + m_storagePath.string() + ": " +
                                        detail);
    };

    std::error_code ec;
    if (std::filesystem::is_directory(target, ec)) {
        throw common::StorageException("Storage path is a directory, not a file: " +
                                       target.string());
    }
    if (!std::filesystem::exists(target, ec)) {
        return {};
    }

    std::ifstream file(target, std::ios::binary);
    if (!file.is_open()) {
        throw common::StorageException("Failed to open storage file: " + target.string());
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        throw common::StorageException("Failed to read storage file: " + target.string());
    }
    std::string content = buffer.str();

    // Strip UTF-8 BOM if present
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    if (!isValidUtf8(content)) {
        throw corrupt("invalid UTF-8 byte sequence.");
    }

    if (std::all_of(content.begin(), content.end(),
                    [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; })) {
        return {};
    }

    const size_t firstNonWs = content.find_first_not_of(" \t\n\r");
    if (firstNonWs == std::string::npos || content[firstNonWs] != '[') {
        throw corrupt("expected JSON array starting with '['.");
    }

    std::vector<Snippet> snippets;
    std::unordered_set<uint64_t> seenIds;
    std::unordered_set<std::string> seenTitlesLower;
    size_t pos = firstNonWs + 1;
    bool expectComma = false;
    bool closedArray = false;

    while (pos < content.size()) {
        while (pos < content.size() && std::isspace(static_cast<unsigned char>(content[pos]))) {
            pos++;
        }
        if (pos >= content.size()) {
            break;
        }

        if (content[pos] == ']') {
            closedArray = true;
            pos++;
            break;
        }

        if (expectComma) {
            if (content[pos] != ',') {
                throw corrupt("expected ',' or ']' after object.");
            }
            pos++;
            while (pos < content.size() && std::isspace(static_cast<unsigned char>(content[pos]))) {
                pos++;
            }
            if (pos >= content.size()) {
                throw corrupt("unexpected end of file after comma.");
            }
            if (content[pos] == ']') {
                throw corrupt("trailing comma before ']' is not permitted.");
            }
        }

        if (content[pos] != '{') {
            throw corrupt("expected '{' for snippet object.");
        }

        // Find the matching '}' (string-aware, single forward pass). The object text is then
        // fully validated by parseJsonObject.
        size_t end = pos + 1;
        int depth = 1;
        bool inStr = false;
        while (end < content.size() && depth > 0) {
            const char c = content[end];
            if (inStr) {
                if (c == '\\') {
                    ++end;  // skip the escaped character
                } else if (c == '"') {
                    inStr = false;
                }
            } else if (c == '"') {
                inStr = true;
            } else {
                if (c == '{') {
                    depth++;
                    if (depth > kMaxJsonDepth) {
                        throw corrupt("JSON nesting depth exceeds maximum supported (64).");
                    }
                } else if (c == '}') {
                    depth--;
                }
            }
            end++;
        }
        if (depth != 0) {
            throw corrupt("truncated or unclosed JSON object.");
        }

        auto fields = parseJsonObject(content.substr(pos, end - pos));
        auto field = [&fields](const char* name) -> std::string* {
            auto it = fields.find(name);
            return it == fields.end() ? nullptr : &it->second;
        };
        // Required non-empty string field; throws "missing mandatory '<name>' field" otherwise.
        auto requireField = [&](const char* name) -> std::string {
            std::string* value = field(name);
            if (value == nullptr || value->empty()) {
                throw corrupt(std::string("snippet object missing mandatory '") + name +
                              "' field.");
            }
            return std::move(*value);
        };

        const std::string* idField = field("id");
        if (idField == nullptr) {
            throw corrupt("snippet object missing required numeric 'id' field.");
        }
        const std::string& idStr = *idField;
        if (idStr.empty()) {
            throw corrupt("snippet object has empty 'id' field.");
        }

        uint64_t id = 0;
        const auto [idEnd, parseEc] =
            std::from_chars(idStr.data(), idStr.data() + idStr.size(), id);
        if (parseEc != std::errc{} || idEnd != idStr.data() + idStr.size()) {
            throw corrupt("invalid numeric ID '" + idStr + "'.");
        }
        if (id == 0) {
            throw corrupt("snippet id must be greater than zero.");
        }
        if (!seenIds.insert(id).second) {
            throw corrupt("duplicate snippet ID " + std::to_string(id) + ".");
        }

        std::string title = requireField("title");
        if (!seenTitlesLower.insert(common::toLower(title)).second) {
            throw corrupt("duplicate snippet title '" + title + "'.");
        }

        std::string language = requireField("language");
        std::string code = requireField("code");

        const std::string* tagsField = field("tags");
        if (tagsField == nullptr) {
            throw corrupt("snippet object missing mandatory 'tags' field.");
        }
        std::vector<std::string> tags = parseJsonArray(*tagsField);

        // Legacy/hand-written records may omit timestamps; backfill so they persist on next save.
        const std::string* createdField = field("createdAt");
        std::string createdAt =
            createdField && !createdField->empty() ? *createdField : getIsoTimestamp();
        const std::string* updatedField = field("updatedAt");
        std::string updatedAt = updatedField && !updatedField->empty() ? *updatedField : createdAt;

        snippets.emplace_back(id, std::move(title), std::move(language), std::move(code),
                              std::move(tags), std::move(createdAt), std::move(updatedAt));

        pos = end;
        expectComma = true;
    }

    if (!closedArray) {
        throw corrupt("expected JSON array ending with ']'.");
    }

    while (pos < content.size()) {
        if (!std::isspace(static_cast<unsigned char>(content[pos]))) {
            throw corrupt("unexpected characters after closing ']'.");
        }
        pos++;
    }

    return snippets;
}

void JsonSnippetRepository::saveToFile(const std::filesystem::path& target,
                                       const std::vector<Snippet>& snippets) const {
    std::error_code ec;
    if (target.has_parent_path()) {
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) {
            throw common::StorageException("Unable to create directory: " +
                                           target.parent_path().string());
        }
    }
    if (std::filesystem::is_directory(target, ec)) {
        throw common::StorageException("Storage target is a directory, not a file: " +
                                       target.string());
    }

    std::filesystem::perms targetPerms =
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
    if (std::filesystem::exists(target, ec)) {
        targetPerms = std::filesystem::status(target, ec).permissions();
    }

    static std::atomic<uint64_t> s_fileSeq{0};
    auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path parentDir =
        target.has_parent_path() ? target.parent_path() : std::filesystem::current_path();
    std::filesystem::path tmpPath =
        parentDir / ("." + target.filename().string() + ".tmp." + std::to_string(nowTicks) + "_" +
                     std::to_string(++s_fileSeq));

    struct TempFileGuard {
        std::filesystem::path path;
        bool committed{false};
        ~TempFileGuard() {
            if (!committed) {
                std::error_code rmEc;
                std::filesystem::remove(path, rmEc);
            }
        }
    } guard{tmpPath};

    std::string payload = serializeSnippets(snippets);

#if defined(_WIN32)
    HANDLE hFile = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (attempt > 0) {
            nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
            tmpPath = parentDir / ("." + target.filename().string() + ".tmp." +
                                   std::to_string(nowTicks) + "_" + std::to_string(++s_fileSeq));
            guard.path = tmpPath;
        }
        std::wstring wTmp = tmpPath.wstring();
        hFile = CreateFileW(wTmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            break;
        }
        DWORD err = GetLastError();
        if (err != ERROR_FILE_EXISTS) {
            break;
        }
    }
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        throw common::StorageException(
            "Failed to exclusively create temporary storage file on Windows: " + tmpPath.string() +
            " (Error " + std::to_string(err) + ")");
    }
    const char* ptr = payload.data();
    size_t remaining = payload.size();
    while (remaining > 0) {
        DWORD written = 0;
        DWORD toWrite = static_cast<DWORD>(std::min<size_t>(remaining, 65536));
        if (!WriteFile(hFile, ptr, toWrite, &written, nullptr) || written == 0) {
            DWORD err = GetLastError();
            CloseHandle(hFile);
            throw common::StorageException(
                "Failed to write to temporary storage file on Windows: " + tmpPath.string() +
                " (Error " + std::to_string(err) + ")");
        }
        ptr += written;
        remaining -= written;
    }
    FlushFileBuffers(hFile);
    CloseHandle(hFile);
#else
    int fd = -1;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (attempt > 0) {
            nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
            tmpPath = parentDir / ("." + target.filename().string() + ".tmp." +
                                   std::to_string(nowTicks) + "_" + std::to_string(++s_fileSeq));
            guard.path = tmpPath;
        }
        fd = ::open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd >= 0) {
            break;
        }
        if (errno != EEXIST) {
            break;
        }
    }
    if (fd < 0) {
        throw common::StorageException("Failed to exclusively create temporary storage file: " +
                                       tmpPath.string() + ": " + std::strerror(errno));
    }
    const char* ptr = payload.data();
    size_t remaining = payload.size();
    while (remaining > 0) {
        ssize_t written = ::write(fd, ptr, remaining);
        if (written < 0) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            ::close(fd);
            throw common::StorageException("Failed to write to temporary storage file: " +
                                           tmpPath.string() + ": " + std::strerror(err));
        }
        ptr += written;
        remaining -= static_cast<size_t>(written);
    }
#if defined(__APPLE__) && defined(F_FULLFSYNC)
    if (::fcntl(fd, F_FULLFSYNC) < 0) {
        ::fsync(fd);
    }
#elif defined(__linux__) || defined(__APPLE__) || defined(__unix__) || defined(_POSIX_VERSION)
    ::fsync(fd);
#endif
    ::close(fd);
#endif

    // Best-effort permission restoration; if setting permissions fails (e.g. on exotic or
    // restricted filesystems), the 0600 mode set during open() provides a safe, conservative
    // fallback.
    std::filesystem::permissions(tmpPath, targetPerms, std::filesystem::perm_options::replace, ec);

#if defined(_WIN32)
    std::wstring wTmp = tmpPath.wstring();
    std::wstring wDest = target.wstring();
    bool replaced = false;
    DWORD lastErr = 0;
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (MoveFileExW(wTmp.c_str(), wDest.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            replaced = true;
            break;
        }
        lastErr = GetLastError();
        if (lastErr == ERROR_SHARING_VIOLATION || lastErr == ERROR_ACCESS_DENIED) {
            Sleep(50);
            continue;
        }
        break;
    }
    if (!replaced) {
        throw common::StorageException(
            "Failed to atomically replace storage file on Windows (Error " +
            std::to_string(lastErr) + ")");
    }
#else
    std::filesystem::rename(tmpPath, target, ec);
    if (ec) {
        throw common::StorageException("Failed to atomically replace storage file: " +
                                       ec.message());
    }
    // Best-effort directory fsync to ensure directory entry durability on POSIX
    if (parentDir.is_absolute() || !parentDir.empty()) {
        int dirFd = ::open(parentDir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dirFd >= 0) {
            ::fsync(dirFd);
            ::close(dirFd);
        }
    }
#endif

    guard.committed = true;
}

std::vector<Snippet> JsonSnippetRepository::getAll() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadFromFile(resolveTarget());
}

std::optional<Snippet> JsonSnippetRepository::getById(uint64_t id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto snippets = loadFromFile(resolveTarget());
    auto it = std::find_if(snippets.begin(), snippets.end(),
                           [id](const Snippet& s) { return s.getId() == id; });
    if (it == snippets.end()) {
        return std::nullopt;
    }
    return std::move(*it);
}

std::optional<Snippet> JsonSnippetRepository::getByTitleUnlocked(const std::string& title) const {
    auto snippets = loadFromFile(resolveTarget());
    auto it = std::find_if(snippets.begin(), snippets.end(), [&title](const Snippet& s) {
        return common::equalsIgnoreCase(s.getTitle(), title);
    });
    if (it == snippets.end()) {
        return std::nullopt;
    }
    return std::move(*it);
}

std::optional<Snippet> JsonSnippetRepository::getByTitle(const std::string& title) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getByTitleUnlocked(title);
}

bool JsonSnippetRepository::exists(const std::string& title) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getByTitleUnlocked(title).has_value();
}

size_t JsonSnippetRepository::count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadFromFile(resolveTarget()).size();
}

uint64_t JsonSnippetRepository::save(Snippet& snippet) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto target = resolveTarget();
    FileLock fileLock(target);
    auto snippets = loadFromFile(target);
    const std::string now = getIsoTimestamp();

    // Copy-modify-commit: the caller's snippet is only updated after persistence succeeds.
    Snippet working = snippet;

    // Enforce title uniqueness (case-insensitive) atomically under the file lock.
    for (const auto& s : snippets) {
        if (common::equalsIgnoreCase(s.getTitle(), working.getTitle()) &&
            s.getId() != working.getId()) {
            throw common::ValidationException("A snippet with title '" + working.getTitle() +
                                              "' already exists.");
        }
    }

    auto existing = working.getId() == 0
                        ? snippets.end()
                        : std::find_if(snippets.begin(), snippets.end(), [&](const Snippet& s) {
                              return s.getId() == working.getId();
                          });

    if (working.getId() == 0) {
        uint64_t maxId = 0;
        for (const auto& s : snippets) {
            maxId = std::max(maxId, s.getId());
        }
        if (maxId == std::numeric_limits<uint64_t>::max()) {
            throw common::StorageException("Maximum snippet ID reached (uint64 overflow).");
        }
        working.setId(maxId + 1);
    }

    if (working.getCreatedAt().empty()) {
        working.setCreatedAt(existing != snippets.end() ? existing->getCreatedAt() : now);
    }
    working.setUpdatedAt(now);

    if (existing != snippets.end()) {
        *existing = working;
    } else {
        snippets.push_back(working);
    }

    saveToFile(target, snippets);
    snippet = std::move(working);
    return snippet.getId();
}

bool JsonSnippetRepository::removeWhere(const std::function<bool(const Snippet&)>& predicate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto target = resolveTarget();
    FileLock fileLock(target);
    auto snippets = loadFromFile(target);
    auto it = std::remove_if(snippets.begin(), snippets.end(), predicate);
    if (it == snippets.end()) {
        return false;
    }
    snippets.erase(it, snippets.end());
    renumberSequentially(snippets);
    saveToFile(target, snippets);
    return true;
}

bool JsonSnippetRepository::removeById(uint64_t id) {
    return removeWhere([id](const Snippet& s) { return s.getId() == id; });
}

bool JsonSnippetRepository::removeByTitle(const std::string& title) {
    return removeWhere(
        [&title](const Snippet& s) { return common::equalsIgnoreCase(s.getTitle(), title); });
}

}  // namespace snipcli::model
