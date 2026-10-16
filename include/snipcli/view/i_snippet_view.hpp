#pragma once

#include "snipcli/model/snippet.hpp"
#include <string>
#include <utility>
#include <vector>

namespace snipcli::view {

/**
 * @brief Abstract View interface defining presentation contracts for SnipCLI.
 */
class ISnippetView {
public:
    virtual ~ISnippetView() = default;

    virtual void renderSnippet(const model::Snippet& snippet) = 0;
    virtual void renderSnippetList(const std::vector<model::Snippet>& snippets) = 0;
    virtual void renderMessage(const std::string& message) = 0;
    virtual void renderSuccess(const std::string& message) = 0;
    virtual void renderError(const std::string& message) = 0;
    virtual void renderStats(
        size_t totalSnippets, 
        size_t totalLines, 
        const std::vector<std::pair<std::string, size_t>>& languageBreakdown) = 0;
};

} // namespace snipcli::view
