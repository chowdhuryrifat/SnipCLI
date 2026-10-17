#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"

namespace snipcli::model {

/**
 * @brief Interface Segregation Principle (ISP): Read-only operations contract.
 *
 * Exposes non-mutating query methods for retrieving snippets from storage.
 * Consumed by read-only controllers (e.g., ListCommand, GetCommand, SearchCommand,
 * StatsCommand) to guarantee they cannot inadvertently alter repository state.
 */
class ISnippetReader {
public:
    virtual ~ISnippetReader() = default;

    /**
     * @brief Retrieves all snippets stored in the repository.
     * @return std::vector<Snippet> Collection of all stored snippet entities.
     * @throws common::StorageException If reading from underlying storage fails.
     */
    [[nodiscard]] virtual std::vector<Snippet> getAll() const = 0;

    /**
     * @brief Searches for a snippet matching a unique numeric identifier.
     * @param id The unique snippet ID to look up.
     * @return std::optional<Snippet> The snippet if found, or std::nullopt.
     * @throws common::StorageException If reading from underlying storage fails.
     */
    [[nodiscard]] virtual std::optional<Snippet> getById(uint64_t id) const = 0;

    /**
     * @brief Searches for a snippet matching an exact unique title.
     * @param title The exact title string to match.
     * @return std::optional<Snippet> The snippet if found, or std::nullopt.
     * @throws common::StorageException If reading from underlying storage fails.
     */
    [[nodiscard]] virtual std::optional<Snippet> getByTitle(const std::string& title) const = 0;

    /**
     * @brief Checks whether a snippet with the given title exists in storage.
     * @param title The title string to check.
     * @return bool True if a snippet with the exact title exists; false otherwise.
     * @throws common::StorageException If querying storage fails.
     */
    [[nodiscard]] virtual bool exists(const std::string& title) const = 0;

    /**
     * @brief Counts the total number of snippets currently stored.
     * @return size_t Total number of stored snippets.
     * @throws common::StorageException If querying storage fails.
     */
    [[nodiscard]] virtual size_t count() const = 0;
};

/**
 * @brief Interface Segregation Principle (ISP): Write/mutation operations contract.
 *
 * Exposes state-mutating operations (insert, update, delete) on snippet storage.
 * Separated from ISnippetReader so commands requiring mutation do not impose
 * mutation dependencies on pure query commands.
 */
class ISnippetWriter {
public:
    virtual ~ISnippetWriter() = default;

    /**
     * @brief Persists a snippet to storage (insert or update).
     *
     * If snippet.getId() is 0, a new unique ID is assigned and the snippet is inserted.
     * If snippet.getId() > 0, the existing record with that ID is updated.
     *
     * @param snippet The snippet entity to save (ID and updatedAt may be mutated).
     * @return uint64_t The persisted snippet's assigned unique numeric ID.
     * @throws common::StorageException If writing to storage fails.
     */
    [[nodiscard]] virtual uint64_t save(Snippet& snippet) = 0;

    /**
     * @brief Deletes a snippet by its unique numeric identifier.
     * @param id The unique identifier of the snippet to remove.
     * @return bool True if the snippet was found and removed; false otherwise.
     * @throws common::StorageException If modifying storage fails.
     */
    [[nodiscard]] virtual bool removeById(uint64_t id) = 0;

    /**
     * @brief Deletes a snippet by its exact title.
     * @param title The exact title of the snippet to remove.
     * @return bool True if the snippet was found and removed; false otherwise.
     * @throws common::StorageException If modifying storage fails.
     */
    [[nodiscard]] virtual bool removeByTitle(const std::string& title) = 0;
};

/**
 * @brief Full Repository interface combining Reader and Writer capabilities (LSP / DIP).
 *
 * Inherits both ISnippetReader and ISnippetWriter to provide a unified contract
 * for concrete persistence implementations (e.g. JsonSnippetRepository) or mock
 * repositories in unit tests, adhering to Liskov Substitution Principle.
 */
class ISnippetRepository : public ISnippetReader, public ISnippetWriter {
public:
    ~ISnippetRepository() override = default;
};

}  // namespace snipcli::model
