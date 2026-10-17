#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "snipcli/model/snippet.hpp"

namespace snipcli::view {

/**
 * @brief Abstract View interface defining presentation contracts for SnipCLI.
 *
 * Implements the presentation boundary in the CLI Model-View-Controller (MVC) architecture.
 * Completely isolates output formatting (ANSI terminal escape codes, Markdown tables, etc.)
 * from domain models and controller orchestration. Concrete views render to an abstract
 * std::ostream (defaults to std::cout or std::cerr) per project technical rules.
 */
class ISnippetView {
public:
    virtual ~ISnippetView() = default;

    /**
     * @brief Renders a single snippet with full metadata, category tags, and formatted code body.
     * @param snippet The snippet domain entity to display.
     */
    virtual void renderSnippet(const model::Snippet& snippet) = 0;

    /**
     * @brief Renders a collection of snippets in a structured tabular format.
     * @param snippets Collection of snippet domain entities to display.
     */
    virtual void renderSnippetList(const std::vector<model::Snippet>& snippets) = 0;

    /**
     * @brief Renders an informational status message.
     * @param message Text content of the informational message.
     */
    virtual void renderMessage(const std::string& message) = 0;

    /**
     * @brief Renders a positive confirmation or operation success message.
     * @param message Text content describing the successful operation.
     */
    virtual void renderSuccess(const std::string& message) = 0;

    /**
     * @brief Renders an error or failure notification.
     * @param message Text content describing the error.
     */
    virtual void renderError(const std::string& message) = 0;

    /**
     * @brief Renders summary repository statistics and telemetry metrics.
     * @param totalSnippets Total count of snippets stored in the repository.
     * @param totalLines Aggregate line count across all stored snippet bodies.
     * @param languageBreakdown Key-value pairs mapping programming languages to snippet counts.
     */
    virtual void renderStats(
        size_t totalSnippets, size_t totalLines,
        const std::vector<std::pair<std::string, size_t>>& languageBreakdown) = 0;
};

}  // namespace snipcli::view
