#pragma once

#include "snipcli/model/snippet.hpp"
#include <optional>
#include <string>
#include <vector>

namespace snipcli::model {

/**
 * @brief Interface Segregation Principle (ISP): Read-only operations contract.
 */
class ISnippetReader {
public:
    virtual ~ISnippetReader() = default;

    [[nodiscard]] virtual std::vector<Snippet> getAll() const = 0;
    [[nodiscard]] virtual std::optional<Snippet> getById(uint64_t id) const = 0;
    [[nodiscard]] virtual std::optional<Snippet> getByTitle(const std::string& title) const = 0;
    [[nodiscard]] virtual bool exists(const std::string& title) const = 0;
    [[nodiscard]] virtual size_t count() const = 0;
};

/**
 * @brief Interface Segregation Principle (ISP): Write/mutation operations contract.
 */
class ISnippetWriter {
public:
    virtual ~ISnippetWriter() = default;

    virtual uint64_t save(Snippet& snippet) = 0;
    virtual bool removeById(uint64_t id) = 0;
    virtual bool removeByTitle(const std::string& title) = 0;
};

/**
 * @brief Full Repository interface combining Reader and Writer capabilities (LSP / DIP).
 */
class ISnippetRepository : public ISnippetReader, public ISnippetWriter {
public:
    ~ISnippetRepository() override = default;
};

} // namespace snipcli::model
