#pragma once

#include <iostream>

#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::view {

/**
 * @brief Terminal-based presentation layer supporting ANSI colors and stylized box formatting.
 */
class TerminalView : public ISnippetView {
public:
    explicit TerminalView(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr);

    void renderSnippet(const model::Snippet& snippet) override;
    void renderSnippetList(const std::vector<model::Snippet>& snippets) override;
    void renderMessage(const std::string& message) override;
    void renderSuccess(const std::string& message) override;
    void renderError(const std::string& message) override;
    void renderStats(size_t totalSnippets, size_t totalLines,
                     const std::vector<std::pair<std::string, size_t>>& languageBreakdown) override;

private:
    std::ostream& m_out;
    std::ostream& m_err;
};

}  // namespace snipcli::view
