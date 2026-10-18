#include "snipcli/model/json_snippet_repository.hpp"

#include <algorithm>
#include <cctype>
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
                case 'u':
                    if (i + 4 < input.size()) {
                        std::string hexStr = input.substr(i + 1, 4);
                        try {
                            unsigned long codePoint = std::stoul(hexStr, nullptr, 16);
                            if (codePoint < 128) {
                                out += static_cast<char>(codePoint);
                                i += 4;
                                break;
                            }
                        } catch (...) {
                        }
                    }
                    out += next;
                    break;
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
            bool inStr = false;
            while (arrEnd < objStr.size()) {
                if (objStr[arrEnd] == '"' && !isEscaped(objStr, arrEnd)) {
                    inStr = !inStr;
                } else if (!inStr && objStr[arrEnd] == ']') {
                    arrEnd++;
                    break;
                }
                arrEnd++;
            }
            value = objStr.substr(arrStart, arrEnd - arrStart);
            idx = arrEnd;
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
    if (content.empty()) {
        return {};
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
        if (depth == 0) {
            std::string objStr = content.substr(pos, end - pos);
            auto fields = parseJsonObject(objStr);

            std::string idStr = fields["id"];
            std::string title = fields["title"];
            std::string language = fields["language"];
            std::string code = fields["code"];
            std::string tagsStr = fields["tags"];
            std::string createdAt = fields["createdAt"];
            std::string updatedAt = fields["updatedAt"];

            uint64_t id = 0;
            if (!idStr.empty()) {
                try {
                    id = std::stoull(idStr);
                } catch (...) {
                    id = 0;
                }
            }

            std::vector<std::string> tags = parseJsonArray(tagsStr);
            if (!title.empty() || id > 0) {
                snippets.emplace_back(id, std::move(title), std::move(language), std::move(code),
                                      std::move(tags), std::move(createdAt), std::move(updatedAt));
            }
            pos = end;
        } else {
            break;
        }
    }

    return snippets;
}

void JsonSnippetRepository::saveToFile(const std::vector<Snippet>& snippets) const {
    ensureStorageDirectoryExists();
    std::ofstream file(m_storagePath, std::ios::trunc);
    if (!file.is_open()) {
        throw common::StorageException("Failed to write to storage file: " +
                                       m_storagePath.string());
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

    if (snippet.getId() == 0) {
        uint64_t maxId = 0;
        for (const auto& s : snippets) {
            if (s.getId() > maxId)
                maxId = s.getId();
        }
        snippet.setId(maxId + 1);
        if (snippet.getCreatedAt().empty()) {
            snippet.setCreatedAt(now);
        }
        snippet.setUpdatedAt(now);
        snippets.push_back(snippet);
    } else {
        bool found = false;
        for (auto& s : snippets) {
            if (s.getId() == snippet.getId()) {
                if (snippet.getCreatedAt().empty()) {
                    snippet.setCreatedAt(s.getCreatedAt());
                }
                snippet.setUpdatedAt(now);
                s = snippet;
                found = true;
                break;
            }
        }
        if (!found) {
            if (snippet.getCreatedAt().empty()) {
                snippet.setCreatedAt(now);
            }
            snippet.setUpdatedAt(now);
            snippets.push_back(snippet);
        }
    }

    saveToFile(snippets);
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
