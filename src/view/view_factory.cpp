#include "snipcli/view/view_factory.hpp"

#include "snipcli/common/string_utils.hpp"
#include "snipcli/view/markdown_view.hpp"
#include "snipcli/view/terminal_view.hpp"

namespace snipcli::view {

std::unique_ptr<ISnippetView> ViewFactory::create(Format format, std::ostream& outStream,
                                                  std::ostream& errStream) {
    switch (format) {
        case Format::Markdown:
            return std::make_unique<MarkdownView>(outStream, errStream);
        case Format::Terminal:
        default:
            return std::make_unique<TerminalView>(outStream, errStream);
    }
}

std::unique_ptr<ISnippetView> ViewFactory::fromString(const std::string& formatName,
                                                      std::ostream& outStream,
                                                      std::ostream& errStream) {
    const std::string lower = common::toLower(formatName);
    const Format format =
        (lower == "markdown" || lower == "md") ? Format::Markdown : Format::Terminal;
    return create(format, outStream, errStream);
}

}  // namespace snipcli::view
