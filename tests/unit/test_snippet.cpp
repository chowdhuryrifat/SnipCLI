#include <cassert>
#include <iostream>

#include "snipcli/model/snippet.hpp"

void runSnippetTests() {
    std::cout << "[TEST] Running Snippet entity tests..." << std::endl;
    snipcli::model::Snippet snippet(1, "Dijkstra", "cpp", "int main() {\n  return 0;\n}\n",
                                    {"graph", "shortest-path"}, "2026-10-16T10:00:00Z",
                                    "2026-10-16T10:00:00Z");

    if (snippet.getId() != 1 || snippet.getTitle() != "Dijkstra" ||
        snippet.getLanguage() != "cpp" || snippet.getLineCount() != 3 || !snippet.hasTag("graph") ||
        snippet.hasTag("dp")) {
        std::cerr << "Assertion failed in Snippet entity tests!" << std::endl;
        std::exit(1);
    }
    std::cout << "[PASS] Snippet entity tests passed." << std::endl;
}
