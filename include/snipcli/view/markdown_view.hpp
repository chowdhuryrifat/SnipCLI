#pragma once

#include "snipcli/view/i_snippet_view.hpp"
#include <iostream>

namespace snipcli::view {

/**
 * @brief Markdown-based presentation layer generating GitHub-Flavored Markdown tables and blocks.
 */
class MarkdownView : public ISnippetView {
public:
    explicit MarkdownView(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr);

    void renderSnippet(const model::Snippet& snippet) override;
    void renderSnippetList(const std::vector<model::Snippet>& snippets) override;
    void renderMessage(const std::string& message) override;
    void renderSuccess(const std::string& message) override;
    void renderError(const std::string& message) override;
    void renderStats(
        size_t totalSnippets, 
        size_t totalLines, 
        const std::vector<std::pair<std::string, size_t>>& languageBreakdown) override;

private:
    std::ostream& m_out;
    std::ostream& m_err;
};

} // namespace snipcli::view
