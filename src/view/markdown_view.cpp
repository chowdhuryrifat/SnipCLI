#include "snipcli/view/markdown_view.hpp"

namespace snipcli::view {

MarkdownView::MarkdownView(std::ostream& outStream, std::ostream& errStream)
    : m_out(outStream), m_err(errStream) {}

void MarkdownView::renderSnippet(const model::Snippet& snippet) {
    m_out << "# " << snippet.getTitle() << "\n\n";
    m_out << "- **ID:** " << snippet.getId() << "\n";
    m_out << "- **Language:** `" << snippet.getLanguage() << "`\n";
    m_out << "- **Lines:** " << snippet.getLineCount() << "\n";
    m_out << "- **Tags:** ";
    if (snippet.getTags().empty()) {
        m_out << "_none_";
    } else {
        for (size_t i = 0; i < snippet.getTags().size(); ++i) {
            m_out << "`" << snippet.getTags()[i] << "`"
                  << (i + 1 < snippet.getTags().size() ? ", " : "");
        }
    }
    m_out << "\n\n```" << snippet.getLanguage() << "\n";
    m_out << snippet.getCode();
    if (!snippet.getCode().empty() && snippet.getCode().back() != '\n') {
        m_out << "\n";
    }
    m_out << "```\n";
}

void MarkdownView::renderSnippetList(const std::vector<model::Snippet>& snippets) {
    if (snippets.empty()) {
        m_out << "_No snippets found in library._\n";
        return;
    }

    m_out << "| ID | Title | Language | Lines | Tags |\n";
    m_out << "|:---|:---|:---|:---|:---|\n";

    for (const auto& s : snippets) {
        std::string tagStr;
        if (s.getTags().empty()) {
            tagStr = "-";
        } else {
            for (size_t i = 0; i < s.getTags().size(); ++i) {
                tagStr += "`" + s.getTags()[i] + "`";
                if (i + 1 < s.getTags().size()) {
                    tagStr += ", ";
                }
            }
        }

        m_out << "| " << s.getId() << " | " << s.getTitle() << " | " << s.getLanguage() << " | "
              << s.getLineCount() << " | " << tagStr << " |\n";
    }
}

void MarkdownView::renderMessage(const std::string& message) {
    m_out << message << "\n";
}

void MarkdownView::renderRaw(const std::string& content) {
    m_out << content;
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
        m_out << "| " << lang << " | " << count << " |\n";
    }
}

}  // namespace snipcli::view
