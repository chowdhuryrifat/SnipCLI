#include <cassert>
#include <iostream>

#include "snipcli/model/snippet.hpp"

void runSnippetTests() {
    std::cout << "[TEST] Running Snippet entity tests..." << std::endl;

    // Test 1: Full parameterized constructor and basic getters
    snipcli::model::Snippet snippet(1, "Dijkstra", "cpp", "int main() {\n  return 0;\n}\n",
                                    {"graph", "shortest-path"}, "2026-10-16T10:00:00Z",
                                    "2026-10-16T10:00:00Z");

    if (snippet.getId() != 1 || snippet.getTitle() != "Dijkstra" ||
        snippet.getLanguage() != "cpp" || snippet.getLineCount() != 3 || !snippet.hasTag("graph") ||
        !snippet.hasTag("shortest-path") || snippet.hasTag("dp")) {
        std::cerr << "Assertion failed in Snippet parameterized constructor tests!" << std::endl;
        std::exit(1);
    }

    // Test 2: Default constructor and mutation via setters
    snipcli::model::Snippet emptySnippet;
    if (emptySnippet.getId() != 0 || !emptySnippet.getTitle().empty() ||
        emptySnippet.getLineCount() != 0) {
        std::cerr << "Assertion failed in Snippet default constructor tests!" << std::endl;
        std::exit(1);
    }

    emptySnippet.setId(42);
    emptySnippet.setTitle("QuickSort");
    emptySnippet.setLanguage("cpp");
    emptySnippet.setCode("void sort();");  // 1 line without trailing newline
    emptySnippet.setTags({"sorting", "divide_and_conquer"});
    emptySnippet.setCreatedAt("2026-10-17T00:00:00Z");
    emptySnippet.setUpdatedAt("2026-10-17T00:00:00Z");

    if (emptySnippet.getId() != 42 || emptySnippet.getTitle() != "QuickSort" ||
        emptySnippet.getLanguage() != "cpp" || emptySnippet.getCode() != "void sort();" ||
        emptySnippet.getLineCount() != 1 || !emptySnippet.hasTag("sorting") ||
        emptySnippet.hasTag("graph") || emptySnippet.getCreatedAt() != "2026-10-17T00:00:00Z" ||
        emptySnippet.getUpdatedAt() != "2026-10-17T00:00:00Z") {
        std::cerr << "Assertion failed in Snippet setter tests!" << std::endl;
        std::exit(1);
    }

    std::cout << "[PASS] Snippet entity tests passed." << std::endl;
}
