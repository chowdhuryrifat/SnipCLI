#include "snipcli/model/json_snippet_repository.hpp"
#include "snipcli/common/exceptions.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

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
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b";  break;
            case '\f': ss << "\\f";  break;
            case '\n': ss << "\\n";  break;
            case '\r': ss << "\\r";  break;
            case '\t': ss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    ss << c;
                }
                break;
        }
    }
    return ss.str();
}

std::string unescapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            switch (next) {
                case '"':  out += '"'; break;
                case '\\': out += '\\'; break;
                case '/':  out += '/'; break;
                case 'b':  out += '\b'; break;
                case 'f':  out += '\f'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += next; break;
            }
        } else {
            out += input[i];
        }
    }
    return out;
}

std::string extractJsonField(const std::string& objStr, const std::string& fieldName) {
    std::string needle = "\"" + fieldName + "\"";
    size_t pos = objStr.find(needle);
    if (pos == std::string::npos) return "";

    size_t colon = objStr.find(':', pos + needle.size());
    if (colon == std::string::npos) return "";

    size_t valStart = objStr.find_first_not_of(" \t\n\r", colon + 1);
    if (valStart == std::string::npos) return "";

    if (objStr[valStart] == '"') {
        size_t endQuote = valStart + 1;
        while (endQuote < objStr.size()) {
            if (objStr[endQuote] == '"' && objStr[endQuote - 1] != '\\') {
                break;
            }
            endQuote++;
        }
        if (endQuote >= objStr.size()) return "";
        return unescapeJson(objStr.substr(valStart + 1, endQuote - (valStart + 1)));
    } else if (objStr[valStart] == '[') {
        size_t endBracket = objStr.find(']', valStart);
        if (endBracket == std::string::npos) return "";
        return objStr.substr(valStart, endBracket - valStart + 1);
    } else {
        size_t endVal = objStr.find_first_of(",}\n\r", valStart);
        if (endVal == std::string::npos) endVal = objStr.size();
        return objStr.substr(valStart, endVal - valStart);
    }
}

std::vector<std::string> parseJsonArray(const std::string& arrStr) {
    std::vector<std::string> elements;
    size_t i = 0;
    while (i < arrStr.size()) {
        size_t start = arrStr.find('"', i);
        if (start == std::string::npos) break;
        size_t end = start + 1;
        while (end < arrStr.size()) {
            if (arrStr[end] == '"' && arrStr[end - 1] != '\\') break;
            end++;
        }
        if (end >= arrStr.size()) break;
        elements.push_back(unescapeJson(arrStr.substr(start + 1, end - (start + 1))));
        i = end + 1;
    }
    return elements;
}

} // namespace

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
            throw common::StorageException("Unable to create directory: " + m_storagePath.parent_path().string());
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
            if (content[end] == '"' && content[end - 1] != '\\') {
                inStr = !inStr;
            } else if (!inStr) {
                if (content[end] == '{') depth++;
                else if (content[end] == '}') depth--;
            }
            end++;
        }
        if (depth == 0) {
            std::string objStr = content.substr(pos, end - pos);
            std::string idStr = extractJsonField(objStr, "id");
            std::string title = extractJsonField(objStr, "title");
            std::string language = extractJsonField(objStr, "language");
            std::string code = extractJsonField(objStr, "code");
            std::string tagsStr = extractJsonField(objStr, "tags");
            std::string createdAt = extractJsonField(objStr, "createdAt");
            std::string updatedAt = extractJsonField(objStr, "updatedAt");

            uint64_t id = 0;
            if (!idStr.empty()) {
                try {
                    id = std::stoull(idStr);
                } catch (...) {
                    id = 0;
                }
            }

            std::vector<std::string> tags = parseJsonArray(tagsStr);
            snippets.emplace_back(id, title, language, code, tags, createdAt, updatedAt);
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
        throw common::StorageException("Failed to write to storage file: " + m_storagePath.string());
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
            if (t + 1 < tags.size()) file << ", ";
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

std::optional<Snippet> JsonSnippetRepository::getByTitle(const std::string& title) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto snippets = loadFromFile();
    for (const auto& s : snippets) {
        if (s.getTitle() == title) {
            return s;
        }
    }
    return std::nullopt;
}

bool JsonSnippetRepository::exists(const std::string& title) const {
    return getByTitle(title).has_value();
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
            if (s.getId() > maxId) maxId = s.getId();
        }
        snippet.setId(maxId + 1);
        snippet.setUpdatedAt(now);
        snippets.push_back(snippet);
    } else {
        bool found = false;
        for (auto& s : snippets) {
            if (s.getId() == snippet.getId()) {
                snippet.setUpdatedAt(now);
                s = snippet;
                found = true;
                break;
            }
        }
        if (!found) {
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
    auto it = std::remove_if(snippets.begin(), snippets.end(), [id](const Snippet& s) {
        return s.getId() == id;
    });

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
    auto it = std::remove_if(snippets.begin(), snippets.end(), [&title](const Snippet& s) {
        return s.getTitle() == title;
    });

    if (it == snippets.end()) {
        return false;
    }

    snippets.erase(it, snippets.end());
    saveToFile(snippets);
    return true;
}

} // namespace snipcli::model
