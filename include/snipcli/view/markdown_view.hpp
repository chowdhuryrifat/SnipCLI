#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::view {

/**
 * @brief Markdown-based presentation layer generating GitHub-Flavored Markdown (GFM).
 *
 * Implements ISnippetView to format snippet records, tabular catalog listings, and
 * system telemetry into GitHub-Flavored Markdown tables, headers, and code fences.
 * Emits output to an abstract std::ostream, ensuring compatibility with file redirection,
 * headless pipelines, and automated report generation.
 */
class MarkdownView : public ISnippetView {
public:
    /**
     * @brief Constructs a MarkdownView targeting configurable output streams.
     * @param outStream Standard destination stream for formatted markdown (defaults to std::cout).
     * @param errStream Error destination stream for failure notifications (defaults to std::cerr).
     */
    explicit MarkdownView(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr);

    /**
     * @brief Renders a single snippet as a formatted Markdown section with metadata and fenced code
     * block.
     * @param snippet Domain entity containing snippet metadata and source code.
     */
    void renderSnippet(const model::Snippet& snippet) override;

    /**
     * @brief Renders a collection of snippets as a GitHub-Flavored Markdown table.
     * @param snippets Collection of snippets to format in tabular markdown.
     */
    void renderSnippetList(const std::vector<model::Snippet>& snippets) override;

    /**
     * @brief Emits a raw message or unadorned code body to the markdown stream.
     * @param message Text content to write.
     */
    void renderMessage(const std::string& message) override;

    /**
     * @brief Renders raw unformatted content directly to output stream without trailing newline.
     * @param content Raw string content to output.
     */
    void renderRaw(const std::string& content) override;

    /**
     * @brief Emits a markdown blockquote confirmation for successful operations.
     * @param message Informational confirmation text.
     */
    void renderSuccess(const std::string& message) override;

    /**
     * @brief Emits a markdown blockquote error notification to the error stream.
     * @param message Error description text.
     */
    void renderError(const std::string& message) override;

    /**
     * @brief Renders aggregate repository telemetry as markdown statistics and a language breakdown
     * table.
     * @param totalSnippets Total count of stored snippets.
     * @param totalLines Sum of lines across all stored snippets.
     * @param languageBreakdown Pairs mapping language names to their respective snippet counts.
     */
    void renderStats(size_t totalSnippets, size_t totalLines,
                     const std::vector<std::pair<std::string, size_t>>& languageBreakdown) override;

private:
    std::ostream& m_out;
    std::ostream& m_err;
};

}  // namespace snipcli::view
