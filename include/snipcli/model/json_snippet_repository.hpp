#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "snipcli/model/i_snippet_repository.hpp"

namespace snipcli::model {

/**
 * @brief Thread-safe and process-safe snippet repository persisting data in structured JSON format.
 *
 * Implements ISnippetRepository to provide persistent disk storage for Snippet entities.
 * Concurrency & Integrity Architecture:
 * - In-process thread-safety guarded by mutable std::mutex and std::lock_guard.
 * - Cross-process writer serialization enforced via advisory file lock (<storagePath>.lock)
 *   utilizing flock(LOCK_EX | LOCK_NB) on POSIX and LockFileEx(LOCKFILE_EXCLUSIVE_LOCK) on Windows.
 * - Lock acquisition timeout (default 10 seconds, polling every 50 ms; configurable via
 *   SNIPCLI_LOCK_TIMEOUT_MS) throws StorageException if held by a stuck external process.
 * - Reader operations (getAll, getById, getByTitle, count) operate lock-free over atomic
 *   temporary-file replacement (rename on POSIX, MoveFileExW with retry on Windows).
 * - Strict JSON grammar validation, duplicate ID/title quarantine, and UTF-8 encoding integrity.
 * - Automatic ID generation and ISO 8601 timestamp tracking.
 */
class JsonSnippetRepository : public ISnippetRepository {
public:
    /**
     * @brief Resolves the default storage file path across operating systems.
     *
     * Resolution order:
     * 1. Environment variable SNIPCLI_STORAGE_PATH if non-empty.
     * 2. Windows: %APPDATA%/SnipCLI/snippets.json.
     * 3. Linux/POSIX: $HOME/.snipcli/snippets.json (creates parent folder if needed).
     * 4. Current working directory fallback: ./.snipcli_snippets.json.
     *
     * @return std::filesystem::path Resolved absolute or relative path to storage file.
     */
    [[nodiscard]] static std::filesystem::path getDefaultStoragePath();

    /**
     * @brief Parses and validates lock acquisition timeout from environment string.
     * Clamps to [1, 600000] ms; falls back to 10000 ms on invalid/empty/negative/zero.
     */
    [[nodiscard]] static int parseLockTimeoutMs(const char* envVal) noexcept;

    /**
     * @brief Constructs repository using the default resolved storage path.
     * @throws common::StorageException If the storage directory cannot be created.
     */
    JsonSnippetRepository();

    /**
     * @brief Constructs repository with an explicit storage file path.
     * @param storagePath Target filesystem path for the snippets JSON file.
     * @throws common::StorageException If the parent directory cannot be created.
     */
    explicit JsonSnippetRepository(std::filesystem::path storagePath);

    /**
     * @brief Retrieves all snippets stored in the JSON file.
     * @return std::vector<Snippet> List of all stored snippets.
     * @throws common::StorageException If the storage file cannot be opened.
     */
    [[nodiscard]] std::vector<Snippet> getAll() const override;

    /**
     * @brief Looks up a snippet by its unique numeric identifier.
     * @param id The unique snippet ID.
     * @return std::optional<Snippet> The found snippet, or std::nullopt.
     */
    [[nodiscard]] std::optional<Snippet> getById(uint64_t id) const override;

    /**
     * @brief Looks up a snippet by its exact title.
     * @param title The snippet title to search for.
     * @return std::optional<Snippet> The found snippet, or std::nullopt.
     */
    [[nodiscard]] std::optional<Snippet> getByTitle(const std::string& title) const override;

    /**
     * @brief Checks if a snippet with the specified title exists in storage.
     * @param title The snippet title to check.
     * @return bool True if found; false otherwise.
     */
    [[nodiscard]] bool exists(const std::string& title) const override;

    /**
     * @brief Counts the total number of snippets currently stored.
     * @return size_t Count of stored snippets.
     */
    [[nodiscard]] size_t count() const override;

    /**
     * @brief Persists a snippet entity (insert if id=0, update if id>0).
     * @param snippet The snippet to save (mutates ID and updatedAt).
     * @return uint64_t The assigned or existing snippet ID.
     * @throws common::StorageException If saving to file fails.
     */
    [[nodiscard]] uint64_t save(Snippet& snippet) override;

    /**
     * @brief Deletes a snippet by its unique numeric ID.
     * @param id The ID of the snippet to remove.
     * @return bool True if removed; false if not found.
     * @throws common::StorageException If updating file fails.
     */
    [[nodiscard]] bool removeById(uint64_t id) override;

    /**
     * @brief Deletes a snippet by its exact title.
     * @param title The title of the snippet to remove.
     * @return bool True if removed; false if not found.
     * @throws common::StorageException If updating file fails.
     */
    [[nodiscard]] bool removeByTitle(const std::string& title) override;

    /**
     * @brief Retrieves the active filesystem path of the repository storage file.
     * @return const std::filesystem::path& Reference to the storage path.
     */
    [[nodiscard]] const std::filesystem::path& getStoragePath() const noexcept;

private:
    void ensureStorageDirectoryExists() const;
    [[nodiscard]] std::vector<Snippet> loadFromFile() const;
    void saveToFile(const std::vector<Snippet>& snippets) const;
    [[nodiscard]] std::optional<Snippet> getByTitleUnlocked(const std::string& title) const;

    std::filesystem::path m_storagePath;
    mutable std::mutex m_mutex;
};

}  // namespace snipcli::model
