#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"
#include "snipcli/view/markdown_view.hpp"
#include "snipcli/view/terminal_view.hpp"
#include "snipcli/view/view_factory.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] View Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runViewTests() {
    std::cout << "[TEST] Running Multi-Format Presentation & Factory tests..." << std::endl;

    // ---------------------------------------------------------
    // Test 1: Render informational message (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        view.renderMessage("Informational note");
        assertTrue(out.str() == "Informational note\n", "renderMessage output matches");
        assertTrue(err.str().empty(), "renderMessage does not touch error stream");
    }

    // ---------------------------------------------------------
    // Test 2: Render success and error messages (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        view.renderSuccess("Snippet created");
        assertTrue(out.str().find("[✓] Snippet created") != std::string::npos,
                   "renderSuccess contains checkmark prefix");

        view.renderError("Snippet not found");
        assertTrue(err.str().find("[✗] Error: Snippet not found") != std::string::npos,
                   "renderError writes to error stream with error prefix");
    }

    // ---------------------------------------------------------
    // Test 3: Render single snippet with line numbering & tags (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        snipcli::model::Snippet snippet(1, "Dijkstra", "cpp",
                                        "vector<int> dist;\npq.push({0, s});\n",
                                        {"graph", "shortest-path"}, "2026-10-18", "2026-10-18");

        view.renderSnippet(snippet);
        std::string output = out.str();

        assertTrue(output.find("Dijkstra") != std::string::npos, "Snippet title rendered");
        assertTrue(output.find("cpp") != std::string::npos, "Snippet language rendered");
        assertTrue(output.find("#graph #shortest-path") != std::string::npos,
                   "Snippet tags rendered with hashtag prefixes");
        assertTrue(output.find("1 | vector<int> dist;") != std::string::npos,
                   "Line 1 formatted with line number");
        assertTrue(output.find("2 | pq.push({0, s});") != std::string::npos,
                   "Line 2 formatted with line number");
    }

    // ---------------------------------------------------------
    // Test 4: Render empty snippet collection (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        view.renderSnippetList({});
        assertTrue(out.str().find("No snippets found in library") != std::string::npos,
                   "Empty list renders friendly notice");
    }

    // ---------------------------------------------------------
    // Test 5: Render tabular snippet list with alignment (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        std::vector<snipcli::model::Snippet> list = {
            {1, "ShortTitle", "cpp", "int a;", {"tag1"}, "", ""},
            {2,
             "A Very Long Snippet Title That Exceeds Limit",
             "python",
             "pass",
             {"tag2"},
             "",
             ""}};

        view.renderSnippetList(list);
        std::string output = out.str();

        assertTrue(
            output.find("ID") != std::string::npos && output.find("TITLE") != std::string::npos,
            "Table header present");
        assertTrue(output.find("ShortTitle") != std::string::npos, "Short title present");
        assertTrue(output.find("...") != std::string::npos, "Long title truncated with ellipsis");
    }

    // ---------------------------------------------------------
    // Test 6: Render repository statistics (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        std::vector<std::pair<std::string, size_t>> langs = {{"cpp", 15}, {"python", 5}};
        view.renderStats(20, 450, langs);

        std::string output = out.str();
        assertTrue(output.find("Total Snippets") != std::string::npos,
                   "Stats header contains Total Snippets");
        assertTrue(output.find("20") != std::string::npos, "Total snippets count matches");
        assertTrue(output.find("cpp") != std::string::npos &&
                       output.find("15 snippets") != std::string::npos,
                   "Language count for cpp matches");
        assertTrue(output.find("python") != std::string::npos &&
                       output.find("5 snippets") != std::string::npos,
                   "Language count for python matches");
    }

    // ---------------------------------------------------------
    // Test 7: Snippet with trailing newline without empty extra line (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        snipcli::model::Snippet snippet(3, "OneLine", "cpp", "return 0;\n", {}, "", "");
        view.renderSnippet(snippet);

        std::string output = out.str();
        assertTrue(output.find("1 | return 0;") != std::string::npos, "Single line counted");
        assertTrue(output.find("2 |") == std::string::npos, "No phantom second line rendered");
    }

    // ---------------------------------------------------------
    // Test 8: ANSI color toggle enable/disable (TerminalView)
    // ---------------------------------------------------------
    {
        std::ostringstream outColor;
        std::ostringstream errColor;
        snipcli::view::TerminalView viewColor(outColor, errColor, true);
        assertTrue(viewColor.isColorEnabled(), "Color initialized as enabled");

        viewColor.renderSuccess("Colored text");
        assertTrue(outColor.str().find("\033[32m") != std::string::npos,
                   "Green ANSI escape emitted when color enabled");

        // Disable colors
        viewColor.setColorEnabled(false);
        assertTrue(!viewColor.isColorEnabled(), "Color disabled after toggle");

        std::ostringstream outPlain;
        std::ostringstream errPlain;
        snipcli::view::TerminalView viewPlain(outPlain, errPlain, false);

        viewPlain.renderSuccess("Plain text");
        assertTrue(outPlain.str().find("\033[") == std::string::npos,
                   "Zero ANSI escape codes emitted when color disabled");
    }

    // ---------------------------------------------------------
    // Test 9: MarkdownView - Messages & Blockquote Notices
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        mdView.renderMessage("Raw message body");
        assertTrue(out.str() == "Raw message body\n", "Markdown message rendered as plain line");

        mdView.renderSuccess("Record created");
        assertTrue(out.str().find("> **Success:** Record created\n") != std::string::npos,
                   "Markdown success rendered as blockquote");

        mdView.renderError("Record not found");
        assertTrue(err.str().find("> **Error:** Record not found\n") != std::string::npos,
                   "Markdown error rendered to errStream as blockquote");
    }

    // ---------------------------------------------------------
    // Test 10: MarkdownView - Single Snippet Formatting & Code Fences
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        snipcli::model::Snippet snippet(42, "DijkstraAlgorithm", "cpp",
                                        "vector<int> dist(n, INF);\n", {"graph", "sp"}, "", "");
        mdView.renderSnippet(snippet);
        std::string md = out.str();

        assertTrue(md.find("# DijkstraAlgorithm\n\n") != std::string::npos,
                   "Markdown header # Title present");
        assertTrue(md.find("- **ID:** 42\n") != std::string::npos, "Markdown ID list item");
        assertTrue(md.find("- **Language:** `cpp`\n") != std::string::npos,
                   "Markdown Language in backticks");
        assertTrue(md.find("`graph`, `sp`") != std::string::npos,
                   "Markdown tags wrapped in backticks");
        assertTrue(md.find("```cpp\nvector<int> dist(n, INF);\n```\n") != std::string::npos,
                   "Markdown code fence with language specifier");
    }

    // ---------------------------------------------------------
    // Test 11: MarkdownView - Snippet with Empty Tags
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        snipcli::model::Snippet snippet(1, "NoTagSnippet", "python", "x = 10\n", {}, "", "");
        mdView.renderSnippet(snippet);
        std::string md = out.str();

        assertTrue(md.find("- **Tags:** _none_") != std::string::npos,
                   "Empty tags render as _none_");
    }

    // ---------------------------------------------------------
    // Test 12: MarkdownView - Tabular Snippet List
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        std::vector<snipcli::model::Snippet> snippets = {
            {1, "FloydWarshall", "cpp", "int d[N][N];", {"graph"}, "", ""},
            {2, "Knapsack", "cpp", "int dp[W];", {}, "", ""}};

        mdView.renderSnippetList(snippets);
        std::string table = out.str();

        assertTrue(table.find("| ID | Title | Language | Lines | Tags |\n") != std::string::npos,
                   "Table header present");
        assertTrue(table.find("|:---|:---|:---|:---|:---|\n") != std::string::npos,
                   "Table delimiter present");
        assertTrue(table.find("| 1 | FloydWarshall | cpp | 1 | `graph` |") != std::string::npos,
                   "Row 1 correctly formatted");
        assertTrue(table.find("| 2 | Knapsack | cpp | 1 | - |") != std::string::npos,
                   "Row 2 with empty tags rendered with dash");
    }

    // ---------------------------------------------------------
    // Test 13: MarkdownView - Empty Snippet List
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        mdView.renderSnippetList({});
        assertTrue(out.str() == "_No snippets found in library._\n",
                   "Empty library state rendered as italic notice");
    }

    // ---------------------------------------------------------
    // Test 14: MarkdownView - Repository Statistics & Breakdown Table
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::MarkdownView mdView(out, err);

        std::vector<std::pair<std::string, size_t>> breakdown = {{"cpp", 10}, {"python", 3}};
        mdView.renderStats(13, 250, breakdown);
        std::string stats = out.str();

        assertTrue(stats.find("## SnipCLI Repository Statistics\n\n") != std::string::npos,
                   "Markdown stats H2 present");
        assertTrue(stats.find("- **Total Snippets:** 13\n") != std::string::npos,
                   "Total snippets list item");
        assertTrue(stats.find("- **Total Lines of Code:** 250\n") != std::string::npos,
                   "Total lines list item");
        assertTrue(stats.find("| cpp | 10 |\n") != std::string::npos,
                   "Language breakdown table row cpp");
        assertTrue(stats.find("| python | 3 |\n") != std::string::npos,
                   "Language breakdown table row python");
    }

    // ---------------------------------------------------------
    // Test 15: ViewFactory - Enum Creation & Stream Injection
    // ---------------------------------------------------------
    {
        std::ostringstream outTerm;
        std::ostringstream errTerm;
        auto termView = snipcli::view::ViewFactory::create(
            snipcli::view::ViewFactory::Format::Terminal, outTerm, errTerm);
        assertTrue(termView != nullptr, "Factory must create non-null TerminalView");
        termView->renderMessage("terminal test");
        assertTrue(outTerm.str() == "terminal test\n", "TerminalView routes to injected outStream");

        std::ostringstream outMd;
        std::ostringstream errMd;
        auto mdView = snipcli::view::ViewFactory::create(
            snipcli::view::ViewFactory::Format::Markdown, outMd, errMd);
        assertTrue(mdView != nullptr, "Factory must create non-null MarkdownView");
        mdView->renderSuccess("md test");
        assertTrue(outMd.str().find("> **Success:** md test") != std::string::npos,
                   "MarkdownView routes to injected outStream");
    }

    // ---------------------------------------------------------
    // Test 16: ViewFactory - fromString Name Resolution (Case-Insensitive)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;

        // "markdown"
        auto v1 = snipcli::view::ViewFactory::fromString("markdown", out, err);
        v1->renderSuccess("msg");
        assertTrue(out.str().find("> **Success:**") != std::string::npos,
                   "'markdown' resolves to MarkdownView");

        // "MD" uppercase
        out.str("");
        auto v2 = snipcli::view::ViewFactory::fromString("MD", out, err);
        v2->renderSuccess("msg");
        assertTrue(out.str().find("> **Success:**") != std::string::npos,
                   "'MD' resolves to MarkdownView");

        // "Markdown" mixed case
        out.str("");
        auto v3 = snipcli::view::ViewFactory::fromString("Markdown", out, err);
        v3->renderSuccess("msg");
        assertTrue(out.str().find("> **Success:**") != std::string::npos,
                   "'Markdown' resolves to MarkdownView");
    }

    // ---------------------------------------------------------
    // Test 17: ViewFactory - Terminal Default and Fallback Resolution
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;

        // "terminal"
        auto v1 = snipcli::view::ViewFactory::fromString("terminal", out, err);
        v1->renderSuccess("msg");
        assertTrue(out.str().find("[✓]") != std::string::npos,
                   "'terminal' resolves to TerminalView");

        // Unrecognized string fallback to terminal
        out.str("");
        auto v2 = snipcli::view::ViewFactory::fromString("unknown_format", out, err);
        v2->renderSuccess("msg");
        assertTrue(out.str().find("[✓]") != std::string::npos,
                   "Unknown format falls back to TerminalView");

        // Empty string fallback
        out.str("");
        auto v3 = snipcli::view::ViewFactory::fromString("", out, err);
        v3->renderSuccess("msg");
        assertTrue(out.str().find("[✓]") != std::string::npos,
                   "Empty string falls back to TerminalView");
    }

    // ---------------------------------------------------------
    // Test 18: Byte-Exact renderRaw Output (Finding B7)
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView termView(out, err, false);
        termView.renderRaw("int main() { return 0; }");
        assertTrue(
            out.str() == "int main() { return 0; }",
            "renderRaw in TerminalView must output byte-exact content without trailing newline");

        std::ostringstream mdOut;
        std::ostringstream mdErr;
        snipcli::view::MarkdownView mdView(mdOut, mdErr);
        mdView.renderRaw("def hello(): pass");
        assertTrue(
            mdOut.str() == "def hello(): pass",
            "renderRaw in MarkdownView must output byte-exact content without trailing newline");
    }

    // ---------------------------------------------------------
    // Test 19: NO_COLOR Environment Variable Disables Colors (Finding B6)
    // ---------------------------------------------------------
    {
#if defined(_WIN32)
        _putenv("NO_COLOR=1");
#else
        setenv("NO_COLOR", "1", 1);
#endif
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView viewNoColor(out, err, true);
        assertTrue(
            !viewNoColor.isColorEnabled(),
            "TerminalView must disable colors when NO_COLOR environment variable is present");

#if defined(_WIN32)
        _putenv("NO_COLOR=");
#else
        unsetenv("NO_COLOR");
#endif
    }

    // ---------------------------------------------------------
    // Test 20: Cross-Platform CRLF and LF Stream Byte Identity (Finding B7)
    // ---------------------------------------------------------
    {
        std::string crlfData = "line1\r\nline2\r\n";
        std::string lfData = "line1\nline2\n";

        std::ostringstream outCrlf;
        std::ostringstream errCrlf;
        snipcli::view::TerminalView viewCrlf(outCrlf, errCrlf, false);
        viewCrlf.renderRaw(crlfData);
        assertTrue(outCrlf.str() == crlfData,
                   "renderRaw must preserve exact CRLF bytes without alteration");

        std::ostringstream outLf;
        std::ostringstream errLf;
        snipcli::view::TerminalView viewLf(outLf, errLf, false);
        viewLf.renderRaw(lfData);
        assertTrue(outLf.str() == lfData,
                   "renderRaw must preserve exact LF bytes without alteration");
    }

    std::cout << "[PASS] Multi-Format Presentation & Factory tests passed. (20/20 Scenarios "
                 "Verified)"
              << std::endl;
}
