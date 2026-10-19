#pragma once

#include <iostream>

#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::view {

/**
 * @brief Terminal-based presentation layer supporting ANSI colors and stylized tabular formatting.
 *
 * Implements ISnippetView to format snippet domain data for interactive terminal consoles.
 * Features:
 * - Colorized status indicators (bold green for success, bold red for errors, cyan for titles).
 * - Code formatting with right-aligned line numbers and visual separator rules.
 * - Formatted tabular lists with column padding, alignment, and long-title truncation.
 * - Telemetry metrics rendering for repository statistics.
 * - Configurable ANSI color escape sequences supporting plain-text pipe/redirection mode.
 */
class TerminalView : public ISnippetView {
public:
    /**
     * @brief Constructs a TerminalView writing to specified standard or abstract streams.
     * @param outStream Primary output stream (defaults to std::cout).
     * @param errStream Error diagnostic stream (defaults to std::cerr).
     * @param useColors Whether ANSI color escape codes should be emitted (defaults to true).
     */
    explicit TerminalView(std::ostream& outStream = std::cout, std::ostream& errStream = std::cerr,
                          bool useColors = true);

    /**
     * @brief Renders a single snippet with full metadata, category tags, and numbered code lines.
     * @param snippet The snippet domain entity to display.
     */
    void renderSnippet(const model::Snippet& snippet) override;

    /**
     * @brief Renders a collection of snippets in a formatted terminal table.
     * @param snippets Collection of snippet domain entities to display.
     */
    void renderSnippetList(const std::vector<model::Snippet>& snippets) override;

    /**
     * @brief Renders an informational status message.
     * @param message Text content of the informational message.
     */
    void renderMessage(const std::string& message) override;

    /**
     * @brief Renders raw unformatted content directly to output stream without trailing newline.
     * @param content Raw string content to output.
     */
    void renderRaw(const std::string& content) override;

    /**
     * @brief Renders a positive confirmation message prefixed with a green checkmark indicator.
     * @param message Text content describing the successful operation.
     */
    void renderSuccess(const std::string& message) override;

    /**
     * @brief Renders an error notification prefixed with a red error indicator to the error stream.
     * @param message Text content describing the error.
     */
    void renderError(const std::string& message) override;

    /**
     * @brief Renders summary repository statistics and telemetry metrics in a stylized box.
     * @param totalSnippets Total count of snippets stored in the repository.
     * @param totalLines Aggregate line count across all stored snippet bodies.
     * @param languageBreakdown Key-value pairs mapping programming languages to snippet counts.
     */
    void renderStats(size_t totalSnippets, size_t totalLines,
                     const std::vector<std::pair<std::string, size_t>>& languageBreakdown) override;

    /**
     * @brief Enables or disables ANSI color escape codes.
     * @param enabled True to enable ANSI colors; false to emit plain uncolored text.
     */
    void setColorEnabled(bool enabled) noexcept;

    /**
     * @brief Checks whether ANSI color formatting is currently enabled.
     * @return bool True if colors are enabled; false otherwise.
     */
    [[nodiscard]] bool isColorEnabled() const noexcept;

private:
    std::ostream& m_out;
    std::ostream& m_err;
    bool m_useColors{true};

    // Helper functions returning ANSI sequences if colors enabled, or empty string if disabled
    [[nodiscard]] const char* colorReset() const noexcept;
    [[nodiscard]] const char* colorBold() const noexcept;
    [[nodiscard]] const char* colorRed() const noexcept;
    [[nodiscard]] const char* colorGreen() const noexcept;
    [[nodiscard]] const char* colorYellow() const noexcept;
    [[nodiscard]] const char* colorBlue() const noexcept;
    [[nodiscard]] const char* colorCyan() const noexcept;
    [[nodiscard]] const char* colorDim() const noexcept;
};

}  // namespace snipcli::view
