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
#endif

#include "snipcli/common/exceptions.hpp"

namespace snipcli::model {

namespace {

class FileLock {
public:
    explicit FileLock(const std::filesystem::path& storagePath) {
        std::error_code ec;
        std::filesystem::path target = storagePath;
        if (std::filesystem::is_symlink(target, ec)) {
            target = std::filesystem::weakly_canonical(target, ec);
            if (ec) {
                ec.clear();
                target = std::filesystem::canonical(storagePath, ec);
                if (ec) {
                    target = storagePath;
                }
            }
        }
        if (target.has_parent_path()) {
            std::filesystem::create_directories(target.parent_path(), ec);
            if (ec) {
                throw common::StorageException("Unable to create storage directory for lock: " +
                                               target.parent_path().string());
            }
        }
        std::filesystem::path lockPath = target;
        lockPath += ".lock";

        int timeoutMs = 10000;
        if (const char* timeoutEnv = std::getenv("SNIPCLI_LOCK_TIMEOUT_MS")) {
            int val = std::atoi(timeoutEnv);
            if (val > 0) {
                timeoutMs = val;
            }
        }

#if defined(_WIN32)
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
        m_fd = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        if (m_fd < 0) {
            throw common::StorageException("Failed to open lock file: " + lockPath.string() + ": " +
                                           std::strerror(errno));
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

bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::string getIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &now_c);
#else
    gmtime_r(&now_c, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string escapeJson(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':
                ss << "\\\"";
                break;
            case '\\':
                ss << "\\\\";
                break;
            case '\b':
                ss << "\\b";
                break;
            case '\f':
                ss << "\\f";
                break;
            case '\n':
                ss << "\\n";
                break;
            case '\r':
                ss << "\\r";
                break;
            case '\t':
                ss << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(static_cast<unsigned char>(c)) << std::dec;
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

bool isEscaped(const std::string& str, size_t pos) {
    size_t backslashes = 0;
    while (pos > 0 && str[pos - 1] == '\\') {
        backslashes++;
        pos--;
    }
    return (backslashes % 2) != 0;
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
        size_t keyEnd = keyStart;
        while (keyEnd < last && (objStr[keyEnd] != '"' || isEscaped(objStr, keyEnd))) {
            if (static_cast<unsigned char>(objStr[keyEnd]) < 0x20) {
                throw common::StorageException(
                    "Corrupt storage file: unescaped control character in JSON key.");
            }
            keyEnd++;
        }
        if (keyEnd >= last || objStr[keyEnd] != '"') {
            throw common::StorageException(
                "Corrupt storage file: unterminated key string in JSON object.");
        }
        std::string key = objStr.substr(keyStart, keyEnd - keyStart);
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
            size_t valEnd = valStart;
            while (valEnd < last && (objStr[valEnd] != '"' || isEscaped(objStr, valEnd))) {
                if (static_cast<unsigned char>(objStr[valEnd]) < 0x20) {
                    throw common::StorageException(
                        "Corrupt storage file: unescaped control character in JSON string.");
                }
                valEnd++;
            }
            if (valEnd >= last || objStr[valEnd] != '"') {
                throw common::StorageException(
                    "Corrupt storage file: unterminated value string in JSON object.");
            }
            value = unescapeJson(objStr.substr(valStart, valEnd - valStart));
            idx = valEnd + 1;
        } else if (objStr[idx] == '[') {
            size_t arrStart = idx;
            size_t arrEnd = idx + 1;
            int bracketDepth = 1;
            bool inStr = false;
            while (arrEnd < last && bracketDepth > 0) {
                if (objStr[arrEnd] == '"' && !isEscaped(objStr, arrEnd)) {
                    inStr = !inStr;
                } else if (!inStr) {
                    if (objStr[arrEnd] == '[') {
                        bracketDepth++;
                    } else if (objStr[arrEnd] == ']') {
                        bracketDepth--;
                    }
                }
                arrEnd++;
            }
            if (bracketDepth != 0) {
                throw common::StorageException(
                    "Corrupt storage file: unclosed array in JSON object.");
            }
            value = objStr.substr(arrStart, arrEnd - arrStart);
            idx = arrEnd;
        } else if (objStr[idx] == '{') {
            size_t subStart = idx;
            size_t subEnd = idx + 1;
            int braceDepth = 1;
            bool inStr = false;
            while (subEnd < last && braceDepth > 0) {
                if (objStr[subEnd] == '"' && !isEscaped(objStr, subEnd)) {
                    inStr = !inStr;
                } else if (!inStr) {
                    if (objStr[subEnd] == '{') {
                        braceDepth++;
                    } else if (objStr[subEnd] == '}') {
                        braceDepth--;
                    }
                }
                subEnd++;
            }
            if (braceDepth != 0) {
                throw common::StorageException(
                    "Corrupt storage file: unclosed nested object in JSON object.");
            }
            value = objStr.substr(subStart, subEnd - subStart);
            idx = subEnd;
        } else {
            size_t valStart = idx;
            while (idx < last && objStr[idx] != ',' &&
                   !std::isspace(static_cast<unsigned char>(objStr[idx]))) {
                idx++;
            }
            value = objStr.substr(valStart, idx - valStart);
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
        size_t end = i + 1;
        while (end < last && (arrStr[end] != '"' || isEscaped(arrStr, end))) {
            if (static_cast<unsigned char>(arrStr[end]) < 0x20) {
                throw common::StorageException(
                    "Corrupt storage file: unescaped control character in tag string.");
            }
            end++;
        }
        if (end >= last || arrStr[end] != '"') {
            throw common::StorageException(
                "Corrupt storage file: unterminated string in tags array.");
        }
        elements.push_back(unescapeJson(arrStr.substr(i + 1, end - (i + 1))));
        i = end + 1;
        expectComma = true;
    }
    return elements;
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

JsonSnippetRepository::JsonSnippetRepository() : JsonSnippetRepository(getDefaultStoragePath()) {}

JsonSnippetRepository::JsonSnippetRepository(std::filesystem::path storagePath)
    : m_storagePath(std::move(storagePath)) {}

const std::filesystem::path& JsonSnippetRepository::getStoragePath() const noexcept {
    return m_storagePath;
}

void JsonSnippetRepository::ensureStorageDirectoryExists() const {
    std::error_code ec;
    std::filesystem::path target = m_storagePath;
    if (std::filesystem::is_symlink(target, ec)) {
        target = std::filesystem::weakly_canonical(target, ec);
        if (ec) {
            ec.clear();
            target = std::filesystem::canonical(m_storagePath, ec);
            if (ec) {
                target = m_storagePath;
            }
        }
    }
    if (target.has_parent_path()) {
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) {
            throw common::StorageException("Unable to create directory: " +
                                           target.parent_path().string());
        }
    }
}

std::vector<Snippet> JsonSnippetRepository::loadFromFile() const {
    std::error_code ec;
    if (std::filesystem::is_directory(m_storagePath, ec)) {
        throw common::StorageException("Storage path is a directory, not a file: " +
                                       m_storagePath.string());
    }
    if (!std::filesystem::exists(m_storagePath, ec)) {
        return {};
    }

    std::ifstream file(m_storagePath, std::ios::binary);
    if (!file.is_open()) {
        throw common::StorageException("Failed to open storage file: " + m_storagePath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        throw common::StorageException("Failed to read storage file: " + m_storagePath.string());
    }
    std::string content = buffer.str();

    // Strip UTF-8 BOM if present
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    bool emptyOrWhitespace = true;
    for (char c : content) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            emptyOrWhitespace = false;
            break;
        }
    }
    if (emptyOrWhitespace) {
        return {};
    }

    size_t firstNonWs = content.find_first_not_of(" \t\n\r");
    if (firstNonWs == std::string::npos || content[firstNonWs] != '[') {
        throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                       ": expected JSON array starting with '['.");
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
                throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                               ": expected ',' or ']' after object.");
            }
            pos++;
            while (pos < content.size() && std::isspace(static_cast<unsigned char>(content[pos]))) {
                pos++;
            }
            if (pos >= content.size()) {
                throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                               ": unexpected end of file after comma.");
            }
            if (content[pos] == ']') {
                throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                               ": trailing comma before ']' is not permitted.");
            }
        }

        if (content[pos] != '{') {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": expected '{' for snippet object.");
        }

        size_t end = pos + 1;
        int depth = 1;
        bool inStr = false;
        while (end < content.size() && depth > 0) {
            if (content[end] == '"' && !isEscaped(content, end)) {
                inStr = !inStr;
            } else if (!inStr) {
                if (content[end] == '{') {
                    depth++;
                } else if (content[end] == '}') {
                    depth--;
                }
            }
            end++;
        }
        if (depth != 0) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": truncated or unclosed JSON object.");
        }

        std::string objStr = content.substr(pos, end - pos);
        auto fields = parseJsonObject(objStr);

        if (fields.find("id") == fields.end()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object missing required numeric 'id' field.");
        }
        std::string idStr = fields["id"];
        if (idStr.empty()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object has empty 'id' field.");
        }

        uint64_t id = 0;
        auto [p, parseEc] = std::from_chars(idStr.data(), idStr.data() + idStr.size(), id);
        if (parseEc != std::errc{} || p != idStr.data() + idStr.size()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": invalid numeric ID '" + idStr + "'.");
        }
        if (id == 0) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet id must be greater than zero.");
        }
        if (!seenIds.insert(id).second) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": duplicate snippet ID " + std::to_string(id) + ".");
        }

        if (fields.find("title") == fields.end() || fields["title"].empty()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object missing mandatory 'title' field.");
        }
        std::string title = fields["title"];
        std::string titleLower;
        titleLower.reserve(title.size());
        for (char c : title) {
            titleLower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (!seenTitlesLower.insert(titleLower).second) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": duplicate snippet title '" + title + "'.");
        }

        if (fields.find("language") == fields.end() || fields["language"].empty()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object missing mandatory 'language' field.");
        }
        std::string language = fields["language"];

        if (fields.find("code") == fields.end() || fields["code"].empty()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object missing mandatory 'code' field.");
        }
        std::string code = fields["code"];

        if (fields.find("tags") == fields.end()) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": snippet object missing mandatory 'tags' field.");
        }
        std::vector<std::string> tags = parseJsonArray(fields["tags"]);

        std::string createdAt = fields.count("createdAt") && !fields["createdAt"].empty()
                                    ? fields["createdAt"]
                                    : getIsoTimestamp();
        std::string updatedAt = fields.count("updatedAt") && !fields["updatedAt"].empty()
                                    ? fields["updatedAt"]
                                    : createdAt;

        snippets.emplace_back(id, std::move(title), std::move(language), std::move(code),
                              std::move(tags), std::move(createdAt), std::move(updatedAt));

        pos = end;
        expectComma = true;
    }

    if (!closedArray) {
        throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                       ": expected JSON array ending with ']'.");
    }

    while (pos < content.size()) {
        if (!std::isspace(static_cast<unsigned char>(content[pos]))) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": unexpected characters after closing ']'.");
        }
        pos++;
    }

    return snippets;
}

void JsonSnippetRepository::saveToFile(const std::vector<Snippet>& snippets) const {
    ensureStorageDirectoryExists();

    std::error_code ec;
    std::filesystem::path target = m_storagePath;
    if (std::filesystem::is_symlink(target, ec)) {
        target = std::filesystem::weakly_canonical(target, ec);
        if (ec) {
            ec.clear();
            target = std::filesystem::canonical(m_storagePath, ec);
            if (ec) {
                target = m_storagePath;
            }
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

    std::ofstream file(tmpPath, std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        throw common::StorageException("Failed to write to temporary storage file: " +
                                       tmpPath.string());
    }

    std::filesystem::permissions(
        tmpPath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace, ec);

    file << "[\n";
    for (size_t i = 0; i < snippets.size(); ++i) {
        const auto& s = snippets[i];
        file << "  {\n";
        file << "    \"id\": " << s.getId() << ",\n";
        file << "    \"title\": \"" << escapeJson(s.getTitle()) << "\",\n";
        file << "    \"language\": \"" << escapeJson(s.getLanguage()) << "\",\n";
        file << "    \"code\": \"" << escapeJson(s.getCode()) << "\",\n";
        file << "    \"tags\": [";
        const auto& tags = s.getTags();
        for (size_t t = 0; t < tags.size(); ++t) {
            file << "\"" << escapeJson(tags[t]) << "\"";
            if (t + 1 < tags.size())
                file << ", ";
        }
        file << "],\n";
        file << "    \"createdAt\": \"" << escapeJson(s.getCreatedAt()) << "\",\n";
        file << "    \"updatedAt\": \"" << escapeJson(s.getUpdatedAt()) << "\"\n";
        file << "  }" << (i + 1 < snippets.size() ? "," : "") << "\n";
    }
    file << "]\n";

    file.flush();
    if (!file.good()) {
        throw common::StorageException("Stream error during write to temporary file: " +
                                       tmpPath.string());
    }
    file.close();

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
#endif

    guard.committed = true;
}

std::vector<Snippet> JsonSnippetRepository::getAll() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return loadFromFile();
}

std::optional<Snippet> JsonSnippetRepository::getById(uint64_t id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto snippets = loadFromFile();
    for (const auto& s : snippets) {
        if (s.getId() == id) {
            return s;
        }
    }
    return std::nullopt;
}

std::optional<Snippet> JsonSnippetRepository::getByTitleUnlocked(const std::string& title) const {
    auto snippets = loadFromFile();
    for (const auto& s : snippets) {
        if (equalsIgnoreCase(s.getTitle(), title)) {
            return s;
        }
    }
    return std::nullopt;
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
    return loadFromFile().size();
}

uint64_t JsonSnippetRepository::save(Snippet& snippet) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FileLock fl(m_storagePath);
    auto snippets = loadFromFile();
    std::string now = getIsoTimestamp();

    Snippet working = snippet;

    // Enforce atomic title uniqueness (case-insensitive) under file lock
    for (const auto& s : snippets) {
        if (equalsIgnoreCase(s.getTitle(), working.getTitle()) && s.getId() != working.getId()) {
            throw common::ValidationException("A snippet with title '" + working.getTitle() +
                                              "' already exists.");
        }
    }

    if (working.getId() == 0) {
        uint64_t maxId = 0;
        for (const auto& s : snippets) {
            if (s.getId() > maxId)
                maxId = s.getId();
        }
        if (maxId == std::numeric_limits<uint64_t>::max()) {
            throw common::StorageException("Maximum snippet ID reached (uint64 overflow).");
        }
        working.setId(maxId + 1);
        if (working.getCreatedAt().empty()) {
            working.setCreatedAt(now);
        }
        working.setUpdatedAt(now);
        snippets.push_back(working);
    } else {
        bool found = false;
        for (auto& s : snippets) {
            if (s.getId() == working.getId()) {
                if (working.getCreatedAt().empty()) {
                    working.setCreatedAt(s.getCreatedAt());
                }
                working.setUpdatedAt(now);
                s = working;
                found = true;
                break;
            }
        }
        if (!found) {
            if (working.getCreatedAt().empty()) {
                working.setCreatedAt(now);
            }
            working.setUpdatedAt(now);
            snippets.push_back(working);
        }
    }

    saveToFile(snippets);
    // Copy-modify-commit: commit state to caller only after persistence succeeds
    snippet = std::move(working);
    return snippet.getId();
}

bool JsonSnippetRepository::removeById(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FileLock fl(m_storagePath);
    auto snippets = loadFromFile();
    auto it = std::remove_if(snippets.begin(), snippets.end(),
                             [id](const Snippet& s) { return s.getId() == id; });

    if (it == snippets.end()) {
        return false;
    }

    snippets.erase(it, snippets.end());
    saveToFile(snippets);
    return true;
}

bool JsonSnippetRepository::removeByTitle(const std::string& title) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FileLock fl(m_storagePath);
    auto snippets = loadFromFile();
    auto it = std::remove_if(snippets.begin(), snippets.end(), [&title](const Snippet& s) {
        return equalsIgnoreCase(s.getTitle(), title);
    });

    if (it == snippets.end()) {
        return false;
    }

    snippets.erase(it, snippets.end());
    saveToFile(snippets);
    return true;
}

}  // namespace snipcli::model
