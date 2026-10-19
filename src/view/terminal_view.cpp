#include "snipcli/view/terminal_view.hpp"

#include <cstdlib>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

namespace snipcli::view {

namespace {
constexpr const char* kAnsiReset = "\033[0m";
constexpr const char* kAnsiBold = "\033[1m";
constexpr const char* kAnsiRed = "\033[31m";
constexpr const char* kAnsiGreen = "\033[32m";
constexpr const char* kAnsiYellow = "\033[33m";
constexpr const char* kAnsiBlue = "\033[34m";
constexpr const char* kAnsiCyan = "\033[36m";
constexpr const char* kAnsiDim = "\033[2m";
constexpr const char* kEmpty = "";

bool detectColorSupport(std::ostream& outStream, bool useColors) {
    if (!useColors) {
        return false;
    }
    if (const char* noColor = std::getenv("NO_COLOR"); noColor != nullptr && noColor[0] != '\0') {
        return false;
    }
    if (&outStream == &std::cout) {
#if defined(_WIN32)
        if (!_isatty(_fileno(stdout))) {
            return false;
        }
#else
        if (!isatty(fileno(stdout))) {
            return false;
        }
#endif
    }
    return true;
}

}  // namespace

TerminalView::TerminalView(std::ostream& outStream, std::ostream& errStream, bool useColors)
    : m_out(outStream), m_err(errStream), m_useColors(detectColorSupport(outStream, useColors)) {}

void TerminalView::setColorEnabled(bool enabled) noexcept {
    m_useColors = enabled;
}

bool TerminalView::isColorEnabled() const noexcept {
    return m_useColors;
}

const char* TerminalView::colorReset() const noexcept {
    return m_useColors ? kAnsiReset : kEmpty;
}

const char* TerminalView::colorBold() const noexcept {
    return m_useColors ? kAnsiBold : kEmpty;
}

const char* TerminalView::colorRed() const noexcept {
    return m_useColors ? kAnsiRed : kEmpty;
}

const char* TerminalView::colorGreen() const noexcept {
    return m_useColors ? kAnsiGreen : kEmpty;
}

const char* TerminalView::colorYellow() const noexcept {
    return m_useColors ? kAnsiYellow : kEmpty;
}

const char* TerminalView::colorBlue() const noexcept {
    return m_useColors ? kAnsiBlue : kEmpty;
}

const char* TerminalView::colorCyan() const noexcept {
    return m_useColors ? kAnsiCyan : kEmpty;
}

const char* TerminalView::colorDim() const noexcept {
    return m_useColors ? kAnsiDim : kEmpty;
}

void TerminalView::renderSnippet(const model::Snippet& snippet) {
    m_out << colorBold() << colorCyan() << "=== Snippet: " << snippet.getTitle()
          << " ===" << colorReset() << "\n";
    m_out << colorDim() << "ID: " << snippet.getId() << " | Language: " << snippet.getLanguage()
          << " | Lines: " << snippet.getLineCount() << colorReset() << "\n";

    m_out << colorYellow() << "Tags: ";
    const auto& tags = snippet.getTags();
    if (tags.empty()) {
        m_out << "(none)";
    } else {
        for (size_t i = 0; i < tags.size(); ++i) {
            m_out << "#" << tags[i] << (i + 1 < tags.size() ? " " : "");
        }
    }
    m_out << colorReset() << "\n";
    m_out << colorDim() << "----------------------------------------------------" << colorReset()
          << "\n";

    // Print code with line numbers
    std::istringstream stream(snippet.getCode());
    std::string line;
    size_t lineNum = 1;
    while (std::getline(stream, line)) {
        m_out << colorDim() << std::setw(4) << lineNum++ << " | " << colorReset() << line << "\n";
    }
    m_out << colorDim() << "----------------------------------------------------" << colorReset()
          << "\n";
}

void TerminalView::renderSnippetList(const std::vector<model::Snippet>& snippets) {
    if (snippets.empty()) {
        m_out << colorYellow() << "No snippets found in library." << colorReset() << "\n";
        return;
    }

    m_out << colorBold() << std::left << std::setw(6) << "ID" << std::setw(25) << "TITLE"
          << std::setw(12) << "LANGUAGE" << std::setw(10) << "LINES" << "TAGS" << colorReset()
          << "\n";

    m_out << colorDim() << std::string(75, '-') << colorReset() << "\n";

    for (const auto& s : snippets) {
        std::string tagStr;
        for (size_t i = 0; i < s.getTags().size(); ++i) {
            tagStr += s.getTags()[i];
            if (i + 1 < s.getTags().size()) {
                tagStr += ", ";
            }
        }

        std::string displayTitle = s.getTitle();
        if (displayTitle.length() > 23) {
            displayTitle.replace(20, std::string::npos, "...");
        }

        m_out << std::left << std::setw(6) << s.getId() << colorCyan() << std::setw(25)
              << displayTitle << colorReset() << std::setw(12) << s.getLanguage() << std::setw(10)
              << s.getLineCount() << colorDim() << tagStr << colorReset() << "\n";
    }
}

void TerminalView::renderRaw(const std::string& content) {
#if defined(_WIN32)
    if (&m_out == &std::cout) {
        m_out.flush();
        int prevMode = _setmode(_fileno(stdout), _O_BINARY);
        m_out.write(content.data(), static_cast<std::streamsize>(content.size()));
        m_out.flush();
        if (prevMode != -1) {
            _setmode(_fileno(stdout), prevMode);
        }
        return;
    }
#endif
    m_out << content;
    m_out.flush();
}

void TerminalView::renderMessage(const std::string& message) {
    m_out << message << "\n";
}

void TerminalView::renderSuccess(const std::string& message) {
    m_out << colorBold() << colorGreen() << "[✓] " << colorReset() << message << "\n";
}

void TerminalView::renderError(const std::string& message) {
    m_err << colorBold() << colorRed() << "[✗] Error: " << colorReset() << message << "\n";
}

void TerminalView::renderStats(
    size_t totalSnippets, size_t totalLines,
    const std::vector<std::pair<std::string, size_t>>& languageBreakdown) {
    m_out << colorBold() << colorBlue() << "📊 SnipCLI Repository Statistics" << colorReset()
          << "\n";
    m_out << colorDim() << "========================================" << colorReset() << "\n";
    m_out << "  • Total Snippets:      " << colorBold() << totalSnippets << colorReset() << "\n";
    m_out << "  • Total Lines of Code: " << colorBold() << totalLines << colorReset() << "\n";
    m_out << "\n" << colorBold() << "Language Breakdown:" << colorReset() << "\n";

    for (const auto& [lang, count] : languageBreakdown) {
        m_out << "  - " << std::left << std::setw(15) << lang << ": " << count << " snippets\n";
    }
    m_out << colorDim() << "========================================" << colorReset() << "\n";
}

}  // namespace snipcli::view
