#include "snipcli/model/json_snippet_repository.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::model {

namespace {

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
        if (input[i] == '\\' && i + 1 < input.size()) {
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
                        out += next;
                        break;
                    }
                    i += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < input.size() &&
                        input[i + 1] == '\\' && input[i + 2] == 'u') {
                        unsigned lo = 0;
                        if (hex4(input, i + 3, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            i += 6;
                        }
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default:
                    out += next;
                    break;
            }
        } else {
            out += input[i];
        }
    }
    return out;
}

std::unordered_map<std::string, std::string> parseJsonObject(const std::string& objStr) {
    std::unordered_map<std::string, std::string> fields;
    size_t idx = 0;
    while (idx < objStr.size()) {
        while (idx < objStr.size() && (std::isspace(static_cast<unsigned char>(objStr[idx])) ||
                                       objStr[idx] == '{' || objStr[idx] == ',')) {
            idx++;
        }
        if (idx >= objStr.size() || objStr[idx] == '}') {
            break;
        }

        if (objStr[idx] != '"') {
            idx++;
            continue;
        }

        size_t keyStart = idx + 1;
        size_t keyEnd = keyStart;
        while (keyEnd < objStr.size() && (objStr[keyEnd] != '"' || isEscaped(objStr, keyEnd))) {
            keyEnd++;
        }
        if (keyEnd >= objStr.size()) {
            break;
        }
        std::string key = objStr.substr(keyStart, keyEnd - keyStart);
        idx = keyEnd + 1;

        while (idx < objStr.size() && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
            idx++;
        }
        if (idx >= objStr.size() || objStr[idx] != ':') {
            break;
        }
        idx++;  // skip ':'

        while (idx < objStr.size() && std::isspace(static_cast<unsigned char>(objStr[idx]))) {
            idx++;
        }
        if (idx >= objStr.size()) {
            break;
        }

        std::string value;
        if (objStr[idx] == '"') {
            size_t valStart = idx + 1;
            size_t valEnd = valStart;
            while (valEnd < objStr.size() && (objStr[valEnd] != '"' || isEscaped(objStr, valEnd))) {
                valEnd++;
            }
            if (valEnd >= objStr.size()) {
                break;
            }
            value = unescapeJson(objStr.substr(valStart, valEnd - valStart));
            idx = valEnd + 1;
        } else if (objStr[idx] == '[') {
            size_t arrStart = idx;
            size_t arrEnd = idx + 1;
            int bracketDepth = 1;
            bool inStr = false;
            while (arrEnd < objStr.size() && bracketDepth > 0) {
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
            value = objStr.substr(arrStart, arrEnd - arrStart);
            idx = arrEnd;
        } else if (objStr[idx] == '{') {
            size_t subStart = idx;
            size_t subEnd = idx + 1;
            int braceDepth = 1;
            bool inStr = false;
            while (subEnd < objStr.size() && braceDepth > 0) {
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
            value = objStr.substr(subStart, subEnd - subStart);
            idx = subEnd;
        } else {
            size_t valStart = idx;
            while (idx < objStr.size() && objStr[idx] != ',' && objStr[idx] != '}' &&
                   !std::isspace(static_cast<unsigned char>(objStr[idx]))) {
                idx++;
            }
            value = objStr.substr(valStart, idx - valStart);
        }
        fields[key] = std::move(value);
    }
    return fields;
}

std::vector<std::string> parseJsonArray(const std::string& arrStr) {
    std::vector<std::string> elements;
    size_t i = 0;
    while (i < arrStr.size()) {
        size_t start = arrStr.find('"', i);
        if (start == std::string::npos)
            break;
        size_t end = start + 1;
        while (end < arrStr.size() && (arrStr[end] != '"' || isEscaped(arrStr, end))) {
            end++;
        }
        if (end >= arrStr.size())
            break;
        elements.push_back(unescapeJson(arrStr.substr(start + 1, end - (start + 1))));
        i = end + 1;
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
        std::filesystem::path candidate =
            std::filesystem::path(appData) / "SnipCLI" / "snippets.json";
        std::error_code ec;
        if (std::filesystem::exists(candidate.parent_path()) ||
            std::filesystem::create_directories(candidate.parent_path(), ec)) {
            return candidate;
        }
    }
#else
    const char* home = std::getenv("HOME");
    if (home && std::string(home).length() > 0) {
        std::filesystem::path candidate =
            std::filesystem::path(home) / ".snipcli" / "snippets.json";
        std::error_code ec;
        if (std::filesystem::exists(candidate.parent_path()) ||
            std::filesystem::create_directories(candidate.parent_path(), ec)) {
            return candidate;
        }
    }
#endif
    return std::filesystem::current_path() / ".snipcli_snippets.json";
}

JsonSnippetRepository::JsonSnippetRepository() : JsonSnippetRepository(getDefaultStoragePath()) {}

JsonSnippetRepository::JsonSnippetRepository(std::filesystem::path storagePath)
    : m_storagePath(std::move(storagePath)) {
    ensureStorageDirectoryExists();
}

const std::filesystem::path& JsonSnippetRepository::getStoragePath() const noexcept {
    return m_storagePath;
}

void JsonSnippetRepository::ensureStorageDirectoryExists() const {
    if (m_storagePath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(m_storagePath.parent_path(), ec);
        if (ec) {
            throw common::StorageException("Unable to create directory: " +
                                           m_storagePath.parent_path().string());
        }
    }
}

std::vector<Snippet> JsonSnippetRepository::loadFromFile() const {
    if (!std::filesystem::exists(m_storagePath)) {
        return {};
    }

    std::ifstream file(m_storagePath);
    if (!file.is_open()) {
        throw common::StorageException("Failed to open storage file: " + m_storagePath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

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
    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos) {
        size_t end = pos + 1;
        int depth = 1;
        bool inStr = false;
        while (end < content.size() && depth > 0) {
            if (content[end] == '"' && !isEscaped(content, end)) {
                inStr = !inStr;
            } else if (!inStr) {
                if (content[end] == '{')
                    depth++;
                else if (content[end] == '}')
                    depth--;
            }
            end++;
        }
        if (depth != 0) {
            throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                           ": truncated or unclosed JSON object.");
        }

        std::string objStr = content.substr(pos, end - pos);
        auto fields = parseJsonObject(objStr);

        std::string idStr = fields["id"];
        std::string title = fields["title"];
        std::string language = fields["language"];
        std::string code = fields["code"];
        std::string tagsStr = fields["tags"];
        std::string createdAt = fields["createdAt"];
        std::string updatedAt = fields["updatedAt"];

        if (title.empty() || code.empty()) {
            throw common::StorageException(
                "Corrupt storage file at " + m_storagePath.string() +
                ": snippet object missing mandatory 'title' or 'code' field.");
        }

        uint64_t id = 0;
        if (!idStr.empty()) {
            auto [p, ec] = std::from_chars(idStr.data(), idStr.data() + idStr.size(), id);
            if (ec != std::errc{} || p != idStr.data() + idStr.size()) {
                throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                               ": invalid numeric ID '" + idStr + "'.");
            }
        }

        std::vector<std::string> tags = parseJsonArray(tagsStr);
        snippets.emplace_back(id, std::move(title), std::move(language), std::move(code),
                              std::move(tags), std::move(createdAt), std::move(updatedAt));
        pos = end;
    }

    size_t lastNonWs = content.find_last_not_of(" \t\n\r");
    if (lastNonWs == std::string::npos || content[lastNonWs] != ']') {
        throw common::StorageException("Corrupt storage file at " + m_storagePath.string() +
                                       ": expected JSON array ending with ']'.");
    }

    return snippets;
}

void JsonSnippetRepository::saveToFile(const std::vector<Snippet>& snippets) const {
    ensureStorageDirectoryExists();

    std::filesystem::path tmpPath = m_storagePath;
    tmpPath += ".tmp";

    std::ofstream file(tmpPath, std::ios::trunc);
    if (!file.is_open()) {
        throw common::StorageException("Failed to write to temporary storage file: " +
                                       tmpPath.string());
    }

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
        std::error_code ec;
        std::filesystem::remove(tmpPath, ec);
        throw common::StorageException("Stream error during write to temporary file: " +
                                       tmpPath.string());
    }
    file.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, m_storagePath, ec);
    if (ec) {
        std::filesystem::copy_file(tmpPath, m_storagePath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmpPath);
        if (ec) {
            throw common::StorageException("Failed to atomically commit storage file: " +
                                           ec.message());
        }
    }
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
        if (s.getTitle() == title) {
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
    auto snippets = loadFromFile();
    std::string now = getIsoTimestamp();

    Snippet working = snippet;

    // Enforce atomic title uniqueness under m_mutex
    for (const auto& s : snippets) {
        if (s.getTitle() == working.getTitle() && s.getId() != working.getId()) {
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
    auto snippets = loadFromFile();
    auto it = std::remove_if(snippets.begin(), snippets.end(),
                             [&title](const Snippet& s) { return s.getTitle() == title; });

    if (it == snippets.end()) {
        return false;
    }

    snippets.erase(it, snippets.end());
    saveToFile(snippets);
    return true;
}

}  // namespace snipcli::model
