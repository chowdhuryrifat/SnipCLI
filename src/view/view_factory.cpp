#include "snipcli/view/view_factory.hpp"

#include <algorithm>
#include <cctype>

#include "snipcli/view/markdown_view.hpp"
#include "snipcli/view/terminal_view.hpp"

namespace snipcli::view {

namespace {

std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return str;
}

}  // namespace

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
    std::string lower = toLower(formatName);
    if (lower == "markdown" || lower == "md") {
        return create(Format::Markdown, outStream, errStream);
    }
    return create(Format::Terminal, outStream, errStream);
}

}  // namespace snipcli::view
