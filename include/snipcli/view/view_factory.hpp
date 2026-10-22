#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::view {

/**
 * @brief Factory Pattern (GoF): Creates polymorphic ISnippetView instances based on format.
 *
 * Implements the Creational GoF Factory Pattern to decouple consumers from concrete
 * view implementations (TerminalView, MarkdownView). Supports runtime stream injection
 * to facilitate automated unit testing and stream redirection.
 */
class ViewFactory {
public:
    /**
     * @brief Supported presentation formats for SnipCLI rendering.
     */
    enum class Format {
        Terminal,  ///< ANSI color-coded terminal text presentation.
        Markdown   ///< GitHub-Flavored Markdown (GFM) formatted text and tables.
    };

    /**
     * @brief Instantiates a concrete view corresponding to the specified Format enum.
     * @param format The target presentation format (Terminal or Markdown).
     * @param outStream Standard destination output stream (defaults to std::cout).
     * @param errStream Error destination output stream (defaults to std::cerr).
     * @return std::unique_ptr<ISnippetView> Polymorphic view instance.
     */
    [[nodiscard]] static std::unique_ptr<ISnippetView> create(Format format,
                                                              std::ostream& outStream = std::cout,
                                                              std::ostream& errStream = std::cerr);

    /**
     * @brief Parses a format identifier string and constructs the corresponding view.
     *
     * Performs case-insensitive matching. Accepts "markdown", "md", "terminal", "term", "text",
     * "txt". Falls back to Format::Terminal if an unrecognized format name is supplied.
     *
     * @param formatName Name of the format string (e.g., from --format flag).
     * @param outStream Standard destination output stream (defaults to std::cout).
     * @param errStream Error destination output stream (defaults to std::cerr).
     * @return std::unique_ptr<ISnippetView> Polymorphic view instance.
     */
    [[nodiscard]] static std::unique_ptr<ISnippetView> fromString(
        const std::string& formatName, std::ostream& outStream = std::cout,
        std::ostream& errStream = std::cerr);
};

}  // namespace snipcli::view
