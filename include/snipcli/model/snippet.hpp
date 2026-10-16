#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace snipcli::model {

/**
 * @brief Core domain entity representing a code template / snippet.
 *
 * Encapsulates all domain attributes of an algorithmic snippet including
 * its unique identifier, title, programming language, raw source code,
 * classification tags, and ISO 8601 creation/update timestamps.
 *
 * Serves as an immutable/copyable Data Transfer Object (DTO) when passed
 * between Model, View, and Controller layers per project MVC invariants.
 */
class Snippet {
public:
    /**
     * @brief Default constructor initializing an empty snippet with ID 0.
     */
    Snippet() = default;

    /**
     * @brief Constructs a fully initialized Snippet entity.
     * @param id Unique numeric identifier assigned by repository storage (0 if unassigned).
     * @param title Unique human-readable title describing the algorithm or template.
     * @param language Programming language identifier (e.g., "cpp", "python", "java").
     * @param code Full source code body of the snippet.
     * @param tags Collection of category/topic tags (e.g., "graph", "dp", "shortest_path").
     * @param createdAt ISO 8601 UTC timestamp of creation.
     * @param updatedAt ISO 8601 UTC timestamp of last modification.
     */
    Snippet(uint64_t id, std::string title, std::string language, std::string code,
            std::vector<std::string> tags, std::string createdAt, std::string updatedAt);

    /**
     * @brief Retrieves the unique numeric identifier of the snippet.
     * @return uint64_t The snippet ID (0 if not yet persisted).
     */
    [[nodiscard]] uint64_t getId() const noexcept;

    /**
     * @brief Retrieves the title of the snippet.
     * @return const std::string& Reference to the snippet title string.
     */
    [[nodiscard]] const std::string& getTitle() const noexcept;

    /**
     * @brief Retrieves the programming language of the snippet.
     * @return const std::string& Reference to the language identifier.
     */
    [[nodiscard]] const std::string& getLanguage() const noexcept;

    /**
     * @brief Retrieves the source code body of the snippet.
     * @return const std::string& Reference to the code string.
     */
    [[nodiscard]] const std::string& getCode() const noexcept;

    /**
     * @brief Retrieves the list of tags associated with the snippet.
     * @return const std::vector<std::string>& Reference to the vector of tags.
     */
    [[nodiscard]] const std::vector<std::string>& getTags() const noexcept;

    /**
     * @brief Retrieves the ISO 8601 creation timestamp.
     * @return const std::string& Timestamp string.
     */
    [[nodiscard]] const std::string& getCreatedAt() const noexcept;

    /**
     * @brief Retrieves the ISO 8601 last update timestamp.
     * @return const std::string& Timestamp string.
     */
    [[nodiscard]] const std::string& getUpdatedAt() const noexcept;

    /**
     * @brief Computes the number of lines in the snippet code body.
     * @return size_t Total line count (0 for empty code).
     */
    [[nodiscard]] size_t getLineCount() const noexcept;

    /**
     * @brief Sets the snippet unique numeric identifier.
     * @param id The new numeric identifier.
     */
    void setId(uint64_t id) noexcept;

    /**
     * @brief Updates the snippet title.
     * @param title The new title string.
     */
    void setTitle(std::string title);

    /**
     * @brief Updates the programming language of the snippet.
     * @param language The new language string.
     */
    void setLanguage(std::string language);

    /**
     * @brief Updates the source code body of the snippet.
     * @param code The new source code string.
     */
    void setCode(std::string code);

    /**
     * @brief Updates the category tags of the snippet.
     * @param tags The new vector of tags.
     */
    void setTags(std::vector<std::string> tags);

    /**
     * @brief Updates the ISO 8601 timestamp of last modification.
     * @param timestamp The new timestamp string.
     */
    void setUpdatedAt(std::string timestamp);

    /**
     * @brief Checks whether the snippet is tagged with a specific category tag.
     * @param tag The tag string to query.
     * @return bool True if the snippet contains the tag; false otherwise.
     */
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
