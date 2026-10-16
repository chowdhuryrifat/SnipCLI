#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace snipcli::model {

/**
 * @brief Core domain entity representing a code template / snippet.
 */
class Snippet {
public:
    Snippet() = default;

    Snippet(uint64_t id, std::string title, std::string language, std::string code,
            std::vector<std::string> tags, std::string createdAt, std::string updatedAt);

    [[nodiscard]] uint64_t getId() const noexcept;
    [[nodiscard]] const std::string& getTitle() const noexcept;
    [[nodiscard]] const std::string& getLanguage() const noexcept;
    [[nodiscard]] const std::string& getCode() const noexcept;
    [[nodiscard]] const std::vector<std::string>& getTags() const noexcept;
    [[nodiscard]] const std::string& getCreatedAt() const noexcept;
    [[nodiscard]] const std::string& getUpdatedAt() const noexcept;
    [[nodiscard]] size_t getLineCount() const noexcept;

    void setId(uint64_t id) noexcept;
    void setTitle(std::string title);
    void setLanguage(std::string language);
    void setCode(std::string code);
    void setTags(std::vector<std::string> tags);
    void setUpdatedAt(std::string timestamp);

    [[nodiscard]] bool hasTag(const std::string& tag) const noexcept;

private:
    uint64_t m_id{0};
    std::string m_title;
    std::string m_language;
    std::string m_code;
    std::vector<std::string> m_tags;
    std::string m_createdAt;
    std::string m_updatedAt;
};

}  // namespace snipcli::model
