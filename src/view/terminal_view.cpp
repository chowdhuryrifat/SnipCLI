#include "snipcli/view/terminal_view.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <string_view>

#include "view_support.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace snipcli::view {

namespace {

namespace ansi {
constexpr const char* kReset = "\033[0m";
constexpr const char* kBold = "\033[1m";
constexpr const char* kRed = "\033[31m";
constexpr const char* kGreen = "\033[32m";
constexpr const char* kYellow = "\033[33m";
constexpr const char* kBlue = "\033[34m";
constexpr const char* kCyan = "\033[36m";
constexpr const char* kDim = "\033[2m";
}  // namespace ansi

constexpr std::string_view kSnippetRule = "----------------------------------------------------";
constexpr std::string_view kStatsRule = "========================================";

// Fixed list-table column widths; TITLE and LANGUAGE are sized to their content.
constexpr int kIdWidth = 6;
constexpr int kLinesWidth = 10;
constexpr int kColumnGap = 2;

void setupWindowsConsoleVT() {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    for (DWORD handleId : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        HANDLE handle = GetStdHandle(handleId);
        DWORD mode = 0;
        if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode)) {
            SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
}

bool isTerminal(std::FILE* file) {
#if defined(_WIN32)
    return _isatty(_fileno(file)) != 0;
#else
    return isatty(fileno(file)) != 0;
#endif
}

/// Colors are used only when requested, NO_COLOR is unset/empty, and a standard stream is a TTY.
/// Non-standard streams (e.g. std::ostringstream in tests) honour @p useColors directly.
bool detectStreamColorSupport(const std::ostream& stream, bool useColors) {
    if (!useColors) {
        return false;
    }
    if (const char* noColor = std::getenv("NO_COLOR"); noColor != nullptr && noColor[0] != '\0') {
        return false;
    }
    if (&stream == &std::cout) {
        return isTerminal(stdout);
    }
    if (&stream == &std::cerr) {
        return isTerminal(stderr);
    }
    return true;
}

}  // namespace

TerminalView::TerminalView(std::ostream& outStream, std::ostream& errStream, bool useColors)
    : m_out(outStream),
      m_err(errStream),
      m_useOutColors(detectStreamColorSupport(outStream, useColors)),
      m_useErrColors(detectStreamColorSupport(errStream, useColors)) {
    if (m_useOutColors || m_useErrColors) {
        setupWindowsConsoleVT();
    }
}

void TerminalView::setColorEnabled(bool enabled) noexcept {
    m_useOutColors = enabled;
    m_useErrColors = enabled;
}

bool TerminalView::isColorEnabled() const noexcept {
    return m_useOutColors;
}

const char* TerminalView::out(const char* code) const noexcept {
    return m_useOutColors ? code : "";
}

const char* TerminalView::err(const char* code) const noexcept {
    return m_useErrColors ? code : "";
}

void TerminalView::renderSnippet(const model::Snippet& snippet) {
    const auto& tags = snippet.getTags();

    m_out << out(ansi::kBold) << out(ansi::kCyan) << "=== Snippet: " << snippet.getTitle()
          << " ===" << out(ansi::kReset) << "\n";
    m_out << out(ansi::kDim) << "ID: " << snippet.getId()
          << " | Language: " << snippet.getLanguage() << " | Lines: " << snippet.getLineCount()
          << out(ansi::kReset) << "\n";
    m_out << out(ansi::kYellow)
          << "Tags: " << (tags.empty() ? "(none)" : detail::join(tags, " ", "#"))
          << out(ansi::kReset) << "\n";
    m_out << out(ansi::kDim) << kSnippetRule << out(ansi::kReset) << "\n";

    // Code body with right-aligned line numbers. Splitting on '\n' matches std::getline
    // semantics: a trailing newline does not produce an extra empty numbered line.
    std::string_view code = snippet.getCode();
    size_t lineNum = 1;
    while (!code.empty()) {
        const size_t nl = code.find('\n');
        const std::string_view line = code.substr(0, nl);
        m_out << out(ansi::kDim) << std::right << std::setw(4) << lineNum++ << " | "
              << out(ansi::kReset) << line << "\n";
        if (nl == std::string_view::npos) {
            break;
        }
        code.remove_prefix(nl + 1);
    }
    m_out << out(ansi::kDim) << kSnippetRule << out(ansi::kReset) << "\n";
}

void TerminalView::renderSnippetList(const std::vector<model::Snippet>& snippets) {
    if (snippets.empty()) {
        m_out << out(ansi::kYellow) << "No snippets found in library." << out(ansi::kReset) << "\n";
        return;
    }

    // Size the title and language columns to their longest value so nothing is truncated.
    size_t maxTitleLen = std::string_view("TITLE").size();
    size_t maxLangLen = std::string_view("LANGUAGE").size();
    for (const auto& s : snippets) {
        maxTitleLen = std::max(maxTitleLen, s.getTitle().size());
        maxLangLen = std::max(maxLangLen, s.getLanguage().size());
    }
    const int titleWidth = static_cast<int>(maxTitleLen) + kColumnGap;
    const int langWidth = static_cast<int>(maxLangLen) + kColumnGap;
    const size_t ruleWidth = static_cast<size_t>(kIdWidth + titleWidth + langWidth + kLinesWidth) +
                             std::string_view("TAGS").size();

    m_out << out(ansi::kBold) << std::left << std::setw(kIdWidth) << "ID" << std::setw(titleWidth)
          << "TITLE" << std::setw(langWidth) << "LANGUAGE" << std::setw(kLinesWidth) << "LINES"
          << "TAGS" << out(ansi::kReset) << "\n";
    m_out << out(ansi::kDim) << std::string(ruleWidth, '-') << out(ansi::kReset) << "\n";

    for (const auto& s : snippets) {
        m_out << std::left << std::setw(kIdWidth) << s.getId() << out(ansi::kCyan)
              << std::setw(titleWidth) << s.getTitle() << out(ansi::kReset) << std::setw(langWidth)
              << s.getLanguage() << std::setw(kLinesWidth) << s.getLineCount() << out(ansi::kDim)
              << detail::join(s.getTags(), ", ") << out(ansi::kReset) << "\n";
    }
}

void TerminalView::renderRaw(const std::string& content) {
    detail::writeRaw(m_out, content);
}

void TerminalView::renderMessage(const std::string& message) {
    m_out << message << "\n";
}

void TerminalView::renderSuccess(const std::string& message) {
    m_out << out(ansi::kBold) << out(ansi::kGreen) << "[✓] " << out(ansi::kReset) << message
          << "\n";
}

void TerminalView::renderError(const std::string& message) {
    m_err << err(ansi::kBold) << err(ansi::kRed) << "[✗] Error: " << err(ansi::kReset) << message
          << "\n";
}

void TerminalView::renderStats(
    size_t totalSnippets, size_t totalLines,
    const std::vector<std::pair<std::string, size_t>>& languageBreakdown) {
    m_out << out(ansi::kBold) << out(ansi::kBlue) << "📊 SnipCLI Repository Statistics"
          << out(ansi::kReset) << "\n";
    m_out << out(ansi::kDim) << kStatsRule << out(ansi::kReset) << "\n";
    m_out << "  • Total Snippets:      " << out(ansi::kBold) << totalSnippets << out(ansi::kReset)
          << "\n";
    m_out << "  • Total Lines of Code: " << out(ansi::kBold) << totalLines << out(ansi::kReset)
          << "\n";
    m_out << "\n" << out(ansi::kBold) << "Language Breakdown:" << out(ansi::kReset) << "\n";
    for (const auto& [lang, count] : languageBreakdown) {
        m_out << "  - " << std::left << std::setw(15) << lang << ": " << count << " snippets\n";
    }
    m_out << out(ansi::kDim) << kStatsRule << out(ansi::kReset) << "\n";
}

}  // namespace snipcli::view
