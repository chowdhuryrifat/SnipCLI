#include "snipcli/view/view_factory.hpp"

#include "snipcli/view/markdown_view.hpp"
#include "snipcli/view/terminal_view.hpp"

namespace snipcli::view {

std::unique_ptr<ISnippetView> ViewFactory::create(Format format) {
    switch (format) {
        case Format::Markdown:
            return std::make_unique<MarkdownView>();
        case Format::Terminal:
        default:
            return std::make_unique<TerminalView>();
    }
}

std::unique_ptr<ISnippetView> ViewFactory::fromString(const std::string& formatName) {
    if (formatName == "markdown" || formatName == "md") {
        return create(Format::Markdown);
    }
    return create(Format::Terminal);
}

}  // namespace snipcli::view
