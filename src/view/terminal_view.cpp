#include "snipcli/view/terminal_view.hpp"

#include <iomanip>
#include <sstream>

namespace snipcli::view {

namespace {
constexpr const char* kReset = "\033[0m";
constexpr const char* kBold = "\033[1m";
constexpr const char* kRed = "\033[31m";
constexpr const char* kGreen = "\033[32m";
constexpr const char* kYellow = "\033[33m";
constexpr const char* kBlue = "\033[34m";
constexpr const char* kCyan = "\033[36m";
constexpr const char* kDim = "\033[2m";
}  // namespace

TerminalView::TerminalView(std::ostream& outStream, std::ostream& errStream)
    : m_out(outStream), m_err(errStream) {}

void TerminalView::renderSnippet(const model::Snippet& snippet) {
    m_out << kBold << kCyan << "=== Snippet: " << snippet.getTitle() << " ===" << kReset << "\n";
    m_out << kDim << "ID: " << snippet.getId() << " | Language: " << snippet.getLanguage()
          << " | Lines: " << snippet.getLineCount() << kReset << "\n";

    m_out << kYellow << "Tags: ";
    const auto& tags = snippet.getTags();
    for (size_t i = 0; i < tags.size(); ++i) {
        m_out << "#" << tags[i] << (i + 1 < tags.size() ? " " : "");
    }
    m_out << kReset << "\n";
    m_out << kDim << "----------------------------------------------------" << kReset << "\n";

    // Print code with line numbers
    std::istringstream stream(snippet.getCode());
    std::string line;
    size_t lineNum = 1;
    while (std::getline(stream, line)) {
        m_out << kDim << std::setw(4) << lineNum++ << " | " << kReset << line << "\n";
    }
    m_out << kDim << "----------------------------------------------------" << kReset << "\n";
}

void TerminalView::renderSnippetList(const std::vector<model::Snippet>& snippets) {
    if (snippets.empty()) {
        m_out << kYellow << "No snippets found in library." << kReset << "\n";
        return;
    }

    m_out << kBold << std::left << std::setw(6) << "ID" << std::setw(25) << "TITLE" << std::setw(12)
          << "LANGUAGE" << std::setw(10) << "LINES"
          << "TAGS" << kReset << "\n";

    m_out << kDim << std::string(75, '-') << kReset << "\n";

    for (const auto& s : snippets) {
        std::string tagStr;
        for (size_t i = 0; i < s.getTags().size(); ++i) {
            tagStr += s.getTags()[i];
            if (i + 1 < s.getTags().size())
                tagStr += ", ";
        }

        m_out << std::left << std::setw(6) << s.getId() << kCyan << std::setw(25)
              << (s.getTitle().length() > 23 ? s.getTitle().substr(0, 20) + "..." : s.getTitle())
              << kReset << std::setw(12) << s.getLanguage() << std::setw(10) << s.getLineCount()
              << kDim << tagStr << kReset << "\n";
    }
}

void TerminalView::renderMessage(const std::string& message) {
    m_out << message << "\n";
}

void TerminalView::renderSuccess(const std::string& message) {
    m_out << kBold << kGreen << "[✓] " << kReset << message << "\n";
}

void TerminalView::renderError(const std::string& message) {
    m_err << kBold << kRed << "[✗] Error: " << kReset << message << "\n";
}

void TerminalView::renderStats(
    size_t totalSnippets, size_t totalLines,
    const std::vector<std::pair<std::string, size_t>>& languageBreakdown) {
    m_out << kBold << kBlue << "📊 SnipCLI Repository Statistics" << kReset << "\n";
    m_out << kDim << "========================================" << kReset << "\n";
    m_out << "  • Total Snippets:      " << kBold << totalSnippets << kReset << "\n";
    m_out << "  • Total Lines of Code: " << kBold << totalLines << kReset << "\n";
    m_out << "\n" << kBold << "Language Breakdown:" << kReset << "\n";

    for (const auto& [lang, count] : languageBreakdown) {
        m_out << "  - " << std::left << std::setw(15) << lang << ": " << count << " snippets\n";
    }
    m_out << kDim << "========================================" << kReset << "\n";
}

}  // namespace snipcli::view
