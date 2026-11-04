#include "snipcli/view/markdown_view.hpp"

#include <algorithm>

#include "view_support.hpp"

namespace snipcli::view {

namespace {

/// Escapes characters that would break a GFM table cell.
std::string escapeTableCell(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '|') {
            out += "\\|";
        } else if (c == '\n' || c == '\r') {
            out += ' ';
        } else {
            out += c;
        }
    }
    return out;
}

/// Returns a backtick fence longer than any backtick run inside @p code (minimum 3), so code
/// containing ``` cannot terminate the fenced block early.
std::string codeFenceFor(std::string_view code) {
    size_t longestRun = 0;
    size_t run = 0;
    for (char c : code) {
        run = (c == '`') ? run + 1 : 0;
        longestRun = std::max(longestRun, run);
    }
    return std::string(std::max<size_t>(3, longestRun + 1), '`');
}

}  // namespace

MarkdownView::MarkdownView(std::ostream& outStream, std::ostream& errStream)
    : m_out(outStream), m_err(errStream) {}

void MarkdownView::renderSnippet(const model::Snippet& snippet) {
    const auto& tags = snippet.getTags();
    const auto& code = snippet.getCode();
    const std::string fence = codeFenceFor(code);

    m_out << "# " << snippet.getTitle() << "\n\n";
    m_out << "- **ID:** " << snippet.getId() << "\n";
    m_out << "- **Language:** `" << snippet.getLanguage() << "`\n";
    m_out << "- **Lines:** " << snippet.getLineCount() << "\n";
    m_out << "- **Tags:** " << (tags.empty() ? "_none_" : detail::join(tags, ", ", "`", "`"));
    m_out << "\n\n" << fence << snippet.getLanguage() << "\n";
    m_out << code;
    if (!code.empty() && code.back() != '\n') {
        m_out << "\n";
    }
    m_out << fence << "\n";
}

void MarkdownView::renderSnippetList(const std::vector<model::Snippet>& snippets) {
    if (snippets.empty()) {
        m_out << "_No snippets found in library._\n";
        return;
    }

    m_out << "| ID | Title | Language | Lines | Tags |\n";
    m_out << "|:---|:---|:---|:---|:---|\n";
    for (const auto& s : snippets) {
        const std::string tagStr =
            s.getTags().empty() ? "-" : escapeTableCell(detail::join(s.getTags(), ", ", "`", "`"));
        m_out << "| " << s.getId() << " | " << escapeTableCell(s.getTitle()) << " | "
              << escapeTableCell(s.getLanguage()) << " | " << s.getLineCount() << " | " << tagStr
              << " |\n";
    }
}

void MarkdownView::renderMessage(const std::string& message) {
    m_out << message << "\n";
}

void MarkdownView::renderRaw(const std::string& content) {
    detail::writeRaw(m_out, content);
}

void MarkdownView::renderSuccess(const std::string& message) {
    m_out << "> **Success:** " << message << "\n";
}

void MarkdownView::renderError(const std::string& message) {
    m_err << "> **Error:** " << message << "\n";
}

void MarkdownView::renderStats(
    size_t totalSnippets, size_t totalLines,
    const std::vector<std::pair<std::string, size_t>>& languageBreakdown) {
    m_out << "## SnipCLI Repository Statistics\n\n";
    m_out << "- **Total Snippets:** " << totalSnippets << "\n";
    m_out << "- **Total Lines of Code:** " << totalLines << "\n\n";
    m_out << "| Language | Snippet Count |\n";
    m_out << "|:---|:---|\n";
    for (const auto& [lang, count] : languageBreakdown) {
        m_out << "| " << escapeTableCell(lang) << " | " << count << " |\n";
    }
}

}  // namespace snipcli::view
