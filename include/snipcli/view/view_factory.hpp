#pragma once

#include <memory>
#include <string>

#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::view {

/**
 * @brief Factory Pattern (GoF): Creates polymorphic ISnippetView instances based on format.
 */
class ViewFactory {
public:
    enum class Format { Terminal, Markdown };

    [[nodiscard]] static std::unique_ptr<ISnippetView> create(Format format);
    [[nodiscard]] static std::unique_ptr<ISnippetView> fromString(const std::string& formatName);
};

}  // namespace snipcli::view
