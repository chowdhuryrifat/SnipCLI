#include "snipcli/model/snippet.hpp"
#include <algorithm>
#include <sstream>

namespace snipcli::model {

Snippet::Snippet(uint64_t id,
                 std::string title,
                 std::string language,
                 std::string code,
                 std::vector<std::string> tags,
                 std::string createdAt,
                 std::string updatedAt)
    : m_id(id),
      m_title(std::move(title)),
      m_language(std::move(language)),
      m_code(std::move(code)),
      m_tags(std::move(tags)),
      m_createdAt(std::move(createdAt)),
      m_updatedAt(std::move(updatedAt)) {}

uint64_t Snippet::getId() const noexcept {
    return m_id;
}

const std::string& Snippet::getTitle() const noexcept {
    return m_title;
}

const std::string& Snippet::getLanguage() const noexcept {
    return m_language;
}

const std::string& Snippet::getCode() const noexcept {
    return m_code;
}

const std::vector<std::string>& Snippet::getTags() const noexcept {
    return m_tags;
}

const std::string& Snippet::getCreatedAt() const noexcept {
    return m_createdAt;
}

const std::string& Snippet::getUpdatedAt() const noexcept {
    return m_updatedAt;
}

size_t Snippet::getLineCount() const noexcept {
    if (m_code.empty()) {
        return 0;
    }
    size_t lines = static_cast<size_t>(std::count(m_code.begin(), m_code.end(), '\n'));
    if (m_code.back() != '\n') {
        lines++;
    }
    return lines;
}

void Snippet::setId(uint64_t id) noexcept {
    m_id = id;
}

void Snippet::setTitle(std::string title) {
    m_title = std::move(title);
}

void Snippet::setLanguage(std::string language) {
    m_language = std::move(language);
}

void Snippet::setCode(std::string code) {
    m_code = std::move(code);
}

void Snippet::setTags(std::vector<std::string> tags) {
    m_tags = std::move(tags);
}

void Snippet::setUpdatedAt(std::string timestamp) {
    m_updatedAt = std::move(timestamp);
}

bool Snippet::hasTag(const std::string& tag) const noexcept {
    return std::any_of(m_tags.begin(), m_tags.end(), [&tag](const std::string& t) {
        return t == tag;
    });
}

} // namespace snipcli::model
