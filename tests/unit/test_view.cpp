#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"
#include "snipcli/view/terminal_view.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] View Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runViewTests() {
    std::cout << "[TEST] Running TerminalView presentation tests..." << std::endl;

    // ---------------------------------------------------------
    // Test 1: Render informational message
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
    // Test 2: Render success and error messages
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
    // Test 3: Render single snippet with line numbering & tags
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        snipcli::model::Snippet snippet(42, "BinarySearch", "cpp", "int l = 0;\nint r = n - 1;",
                                        {"search", "binary"}, "2026-10-18T00:00:00Z",
                                        "2026-10-18T00:00:00Z");

        view.renderSnippet(snippet);
        std::string result = out.str();

        assertTrue(result.find("=== Snippet: BinarySearch ===") != std::string::npos,
                   "Snippet title header present");
        assertTrue(result.find("ID: 42") != std::string::npos, "Snippet ID present");
        assertTrue(result.find("Language: cpp") != std::string::npos, "Snippet language present");
        assertTrue(result.find("Lines: 2") != std::string::npos, "Snippet line count present");
        assertTrue(result.find("#search #binary") != std::string::npos, "Snippet tags formatted");
        assertTrue(result.find("   1 | int l = 0;") != std::string::npos,
                   "First line numbered with padding");
        assertTrue(result.find("   2 | int r = n - 1;") != std::string::npos,
                   "Second line numbered with padding");
    }

    // ---------------------------------------------------------
    // Test 4: Render single snippet with empty tags
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        snipcli::model::Snippet snippet(1, "NoTags", "python", "pass", {}, "", "");
        view.renderSnippet(snippet);
        assertTrue(out.str().find("Tags: (none)") != std::string::npos,
                   "Empty tags renders (none)");
    }

    // ---------------------------------------------------------
    // Test 5: Render empty snippet list
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        view.renderSnippetList({});
        assertTrue(out.str().find("No snippets found in library.") != std::string::npos,
                   "Empty list renders informative notice");
    }

    // ---------------------------------------------------------
    // Test 6: Render snippet list table with truncation
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        std::vector<snipcli::model::Snippet> snippets = {
            {1, "ShortTitle", "cpp", "int a;", {"tag1"}, "", ""},
            {2,
             "VeryLongSnippetTitleExceedingTwentyThreeChars",
             "python",
             "pass",
             {"tag2", "tag3"},
             "",
             ""}};

        view.renderSnippetList(snippets);
        std::string result = out.str();

        assertTrue(result.find("ID    TITLE                    LANGUAGE    LINES     TAGS") !=
                       std::string::npos,
                   "Table header columns present");
        assertTrue(result.find("ShortTitle") != std::string::npos, "Short title rendered intact");
        assertTrue(result.find("VeryLongSnippetTitle...") != std::string::npos,
                   "Long title truncated with ellipsis");
        assertTrue(result.find("tag2, tag3") != std::string::npos,
                   "Comma-separated tags list formatted");
    }

    // ---------------------------------------------------------
    // Test 7: Render repository telemetry statistics
    // ---------------------------------------------------------
    {
        std::ostringstream out;
        std::ostringstream err;
        snipcli::view::TerminalView view(out, err, false);

        std::vector<std::pair<std::string, size_t>> breakdown = {{"cpp", 15}, {"python", 5}};
        view.renderStats(20, 450, breakdown);
        std::string result = out.str();

        assertTrue(result.find("📊 SnipCLI Repository Statistics") != std::string::npos,
                   "Stats header present");
        assertTrue(result.find("Total Snippets:      20") != std::string::npos,
                   "Total snippets formatted");
        assertTrue(result.find("Total Lines of Code: 450") != std::string::npos,
                   "Total lines formatted");
        assertTrue(result.find("cpp            : 15 snippets") != std::string::npos,
                   "Language breakdown row 1 formatted");
        assertTrue(result.find("python         : 5 snippets") != std::string::npos,
                   "Language breakdown row 2 formatted");
    }

    // ---------------------------------------------------------
    // Test 8: ANSI color emission vs plain text toggle
    // ---------------------------------------------------------
    {
        std::ostringstream outColor;
        std::ostringstream errColor;
        snipcli::view::TerminalView viewColor(outColor, errColor, true);
        assertTrue(viewColor.isColorEnabled(), "Color enabled initially");

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

    std::cout << "[PASS] TerminalView presentation tests passed. (8/8 Scenarios Verified)"
              << std::endl;
}
