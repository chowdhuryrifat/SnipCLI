#pragma once

#include "snipcli/model/i_snippet_repository.hpp"
#include <filesystem>
#include <mutex>

namespace snipcli::model {

/**
 * @brief Concrete snippet repository persisting data in structured JSON format.
 */
class JsonSnippetRepository : public ISnippetRepository {
public:
    explicit JsonSnippetRepository(std::filesystem::path storagePath);

    [[nodiscard]] std::vector<Snippet> getAll() const override;
    [[nodiscard]] std::optional<Snippet> getById(uint64_t id) const override;
    [[nodiscard]] std::optional<Snippet> getByTitle(const std::string& title) const override;
    [[nodiscard]] bool exists(const std::string& title) const override;
    [[nodiscard]] size_t count() const override;

    uint64_t save(Snippet& snippet) override;
    bool removeById(uint64_t id) override;
    bool removeByTitle(const std::string& title) override;

    [[nodiscard]] const std::filesystem::path& getStoragePath() const noexcept;

private:
    void ensureStorageDirectoryExists() const;
    [[nodiscard]] std::vector<Snippet> loadFromFile() const;
    void saveToFile(const std::vector<Snippet>& snippets) const;

    std::filesystem::path m_storagePath;
    mutable std::mutex m_mutex;
};

} // namespace snipcli::model
