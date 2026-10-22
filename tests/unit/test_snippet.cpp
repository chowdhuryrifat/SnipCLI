#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] Snippet Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runSnippetTests() {
    std::cout << "[TEST] Running Snippet entity tests..." << std::endl;

    // ---------------------------------------------------------
    // Scenario 1: Parameterized Constructor & Accessors
    // ---------------------------------------------------------
    {
        std::vector<std::string> tags = {"graph", "shortest-path"};
        snipcli::model::Snippet snippet(1, "Dijkstra", "cpp", "int main() {\n  return 0;\n}\n",
                                        tags, "2026-10-19T00:00:00Z", "2026-10-19T00:00:00Z");

        assertTrue(snippet.getId() == 1, "getId() must return 1");
        assertTrue(snippet.getTitle() == "Dijkstra", "getTitle() must return 'Dijkstra'");
        assertTrue(snippet.getLanguage() == "cpp", "getLanguage() must return 'cpp'");
        assertTrue(snippet.getCode() == "int main() {\n  return 0;\n}\n",
                   "getCode() must return full snippet source");
        assertTrue(snippet.getTags().size() == 2, "getTags() must return 2 tags");
        assertTrue(snippet.getTags()[0] == "graph" && snippet.getTags()[1] == "shortest-path",
                   "Tags must match parameterized inputs");
        assertTrue(snippet.getCreatedAt() == "2026-10-19T00:00:00Z", "createdAt must match");
        assertTrue(snippet.getUpdatedAt() == "2026-10-19T00:00:00Z", "updatedAt must match");
        assertTrue(snippet.getLineCount() == 3, "Line count for 3-line snippet must be 3");
    }

    // ---------------------------------------------------------
    // Scenario 2: Default Constructor Initial State
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet empty;
        assertTrue(empty.getId() == 0, "Default ID must be 0");
        assertTrue(empty.getTitle().empty(), "Default title must be empty");
        assertTrue(empty.getLanguage().empty(), "Default language must be empty");
        assertTrue(empty.getCode().empty(), "Default code must be empty");
        assertTrue(empty.getTags().empty(), "Default tags vector must be empty");
        assertTrue(empty.getCreatedAt().empty(), "Default createdAt must be empty");
        assertTrue(empty.getUpdatedAt().empty(), "Default updatedAt must be empty");
        assertTrue(empty.getLineCount() == 0, "Default line count must be 0");
    }

    // ---------------------------------------------------------
    // Scenario 3: Complete Setter Mutation
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet snippet;
        snippet.setId(42);
        snippet.setTitle("QuickSort");
        snippet.setLanguage("python");
        snippet.setCode("def quicksort(arr):\n    pass\n");
        snippet.setTags({"sorting", "recursion"});
        snippet.setCreatedAt("2026-10-19T01:00:00Z");
        snippet.setUpdatedAt("2026-10-19T02:00:00Z");

        assertTrue(snippet.getId() == 42, "Mutated ID must be 42");
        assertTrue(snippet.getTitle() == "QuickSort", "Mutated title must be 'QuickSort'");
        assertTrue(snippet.getLanguage() == "python", "Mutated language must be 'python'");
        assertTrue(snippet.getCode() == "def quicksort(arr):\n    pass\n",
                   "Mutated code must match");
        assertTrue(snippet.getTags().size() == 2, "Mutated tags count must be 2");
        assertTrue(snippet.getCreatedAt() == "2026-10-19T01:00:00Z",
                   "Mutated createdAt must match");
        assertTrue(snippet.getUpdatedAt() == "2026-10-19T02:00:00Z",
                   "Mutated updatedAt must match");
        assertTrue(snippet.getLineCount() == 2, "Mutated snippet line count must be 2");
    }

    // ---------------------------------------------------------
    // Scenario 4: Line Counting Matrix & Boundary Conditions
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet s;

        // Empty code -> 0 lines
        s.setCode("");
        assertTrue(s.getLineCount() == 0, "Empty code must have 0 lines");

        // 1 line without newline -> 1 line
        s.setCode("int x = 10;");
        assertTrue(s.getLineCount() == 1, "Single line without newline must be 1 line");

        // 1 line with newline -> 1 line
        s.setCode("int x = 10;\n");
        assertTrue(s.getLineCount() == 1, "Single line with newline must be 1 line");

        // 3 lines without trailing newline -> 3 lines
        s.setCode("line 1\nline 2\nline 3");
        assertTrue(s.getLineCount() == 3, "3 lines without trailing newline must be 3 lines");

        // 3 lines with trailing newline -> 3 lines
        s.setCode("line 1\nline 2\nline 3\n");
        assertTrue(s.getLineCount() == 3, "3 lines with trailing newline must be 3 lines");

        // Windows CRLF (\r\n) line endings -> 2 lines
        s.setCode("int a = 1;\r\nint b = 2;\r\n");
        assertTrue(s.getLineCount() == 2, "CRLF terminated code must count 2 lines");

        // Consecutive empty lines -> 4 lines
        s.setCode("\n\n\n\n");
        assertTrue(s.getLineCount() == 4, "4 newlines must count as 4 lines");
    }

    // ---------------------------------------------------------
    // Scenario 5: Tag Predicate (hasTag)
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet snippet(1, "BinarySearch", "cpp", "code",
                                        {"binary-search", "divide-and-conquer"}, "", "");

        assertTrue(snippet.hasTag("binary-search"), "hasTag must return true for existing tag");
        assertTrue(snippet.hasTag("divide-and-conquer"), "hasTag must return true for second tag");
        assertTrue(!snippet.hasTag("graph"), "hasTag must return false for missing tag");
        assertTrue(!snippet.hasTag("BINARY-SEARCH"), "hasTag is case-sensitive exact match");
        assertTrue(!snippet.hasTag(""), "hasTag must return false for empty string");

        snipcli::model::Snippet noTags(2, "NoTags", "cpp", "code", {}, "", "");
        assertTrue(!noTags.hasTag("any"), "hasTag must return false on empty tags vector");
    }

    // ---------------------------------------------------------
    // Scenario 6: Deep Copy Semantics (Copy Constructor & Assignment)
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet original(10, "Original", "cpp", "int a = 1;", {"tag1"}, "ts1",
                                         "ts1");

        // Copy constructor
        snipcli::model::Snippet copyConstructed = original;
        assertTrue(copyConstructed.getId() == 10, "Copy constructor must copy ID");
        assertTrue(copyConstructed.getTitle() == "Original", "Copy constructor must copy title");

        // Mutating copy must NOT mutate original (deep copy)
        copyConstructed.setTitle("MutatedCopy");
        copyConstructed.setTags({"mutated_tag"});
        assertTrue(original.getTitle() == "Original", "Original title must remain unchanged");
        assertTrue(original.getTags().size() == 1 && original.getTags()[0] == "tag1",
                   "Original tags must remain unchanged");

        // Copy assignment
        snipcli::model::Snippet copyAssigned;
        copyAssigned = original;
        assertTrue(copyAssigned.getTitle() == "Original", "Copy assignment must copy title");
        copyAssigned.setId(999);
        assertTrue(original.getId() == 10, "Mutating copy ID must not mutate original");
    }

    // ---------------------------------------------------------
    // Scenario 7: Move Semantics (Move Constructor & Assignment)
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet source(100, "ToMove", "cpp", "long code buffer...", {"move_tag"},
                                       "t1", "t2");

        // Move constructor
        snipcli::model::Snippet moved(std::move(source));
        assertTrue(moved.getId() == 100, "Moved instance must retain ID");
        assertTrue(moved.getTitle() == "ToMove", "Moved instance must retain title");
        assertTrue(moved.getLineCount() == 1, "Moved instance must retain line count");

        // Move assignment
        snipcli::model::Snippet target;
        target = std::move(moved);
        assertTrue(target.getId() == 100, "Move-assigned instance must retain ID");
        assertTrue(target.getTitle() == "ToMove", "Move-assigned instance must retain title");
    }

    // ---------------------------------------------------------
    // Scenario 8: Const-Correctness on Accessors
    // ---------------------------------------------------------
    {
        const snipcli::model::Snippet constSnippet(5, "ConstTitle", "rust", "fn main() {}",
                                                   {"const_tag"}, "2026-10-19T00:00:00Z",
                                                   "2026-10-19T00:00:00Z");

        assertTrue(constSnippet.getId() == 5, "Const getId() callable");
        assertTrue(constSnippet.getTitle() == "ConstTitle", "Const getTitle() callable");
        assertTrue(constSnippet.getLanguage() == "rust", "Const getLanguage() callable");
        assertTrue(constSnippet.getCode() == "fn main() {}", "Const getCode() callable");
        assertTrue(constSnippet.getTags().size() == 1, "Const getTags() callable");
        assertTrue(constSnippet.getCreatedAt() == "2026-10-19T00:00:00Z",
                   "Const getCreatedAt() callable");
        assertTrue(constSnippet.getUpdatedAt() == "2026-10-19T00:00:00Z",
                   "Const getUpdatedAt() callable");
        assertTrue(constSnippet.getLineCount() == 1, "Const getLineCount() callable");
        assertTrue(constSnippet.hasTag("const_tag"), "Const hasTag() callable");
    }

    // ---------------------------------------------------------
    // Scenario 9: Tag Vector Operations (Replace, Clear)
    // ---------------------------------------------------------
    {
        snipcli::model::Snippet s(1, "TagsOps", "cpp", "code", {"tagA", "tagB"}, "", "");
        assertTrue(s.getTags().size() == 2, "Initial 2 tags");

        // Replace tags
        s.setTags({"tagC", "tagD", "tagE"});
        assertTrue(s.getTags().size() == 3, "Replaced with 3 tags");
        assertTrue(!s.hasTag("tagA"), "tagA is no longer present");
        assertTrue(s.hasTag("tagD"), "tagD is present");

        // Clear tags
        s.setTags({});
        assertTrue(s.getTags().empty(), "Tags successfully cleared to empty vector");
    }

    // ---------------------------------------------------------
    // Scenario 10: Special Characters and Formatting in Code
    // ---------------------------------------------------------
    {
        const std::string complexCode =
            "#include <iostream>\n"
            "// Special symbols: \t \"quotes\" \\escapes\\ {braces} [brackets]\n"
            "int main() {\n"
            "    std::cout << \"Hello, SnipCLI!\\n\";\n"
            "    return 0;\n"
            "}\n";

        snipcli::model::Snippet s(7, "ComplexCodeSnippet", "cpp", complexCode, {"syntax"}, "", "");
        assertTrue(s.getCode() == complexCode,
                   "Complex code with tabs, quotes, and escapes preserved");
        assertTrue(s.getLineCount() == 6, "Complex code line count correctly computed as 6");
    }

    std::cout << "[PASS] Snippet entity tests passed. (10/10 Invariants Verified)" << std::endl;
}
