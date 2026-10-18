#include <iostream>
#include <string>
#include <vector>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/model/snippet_validator.hpp"

namespace {

template <typename Func>
void assertThrowsValidationException(const std::string& testName, Func&& func) {
    bool caught = false;
    try {
        func();
    } catch (const snipcli::common::ValidationException&) {
        caught = true;
    } catch (...) {
        std::cerr << "[FAIL] " << testName << " threw an unexpected exception type!" << std::endl;
        std::exit(1);
    }

    if (!caught) {
        std::cerr << "[FAIL] " << testName << " failed to throw ValidationException!" << std::endl;
        std::exit(1);
    }
}

template <typename Func>
void assertNoThrow(const std::string& testName, Func&& func) {
    try {
        func();
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] " << testName << " unexpectedly threw: " << ex.what() << std::endl;
        std::exit(1);
    } catch (...) {
        std::cerr << "[FAIL] " << testName << " unexpectedly threw an unknown exception!" << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runValidatorTests() {
    std::cout << "[TEST] Running Validator tests..." << std::endl;

    // ---------------------------------------------------------
    // Invariant 1: Valid Snippet Passes Validation
    // ---------------------------------------------------------
    assertNoThrow("Valid standard snippet", []() {
        snipcli::model::Snippet valid(1, "Dijkstra_Shortest-Path v2", "c++",
                                      "int n = 10;\nvector<int> d;\n", {"graph", "sp"}, "", "");
        snipcli::model::SnippetValidator::validate(valid);
    });

    // ---------------------------------------------------------
    // Invariant 2: Empty Title Rejection
    // ---------------------------------------------------------
    assertThrowsValidationException("Empty title", []() {
        snipcli::model::Snippet s(1, "", "cpp", "int x = 0;", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // ---------------------------------------------------------
    // Invariant 3: Title Boundary - Exactly 100 Characters (kMaxTitleLength)
    // ---------------------------------------------------------
    assertNoThrow("Title exactly 100 characters boundary", []() {
        std::string maxTitle(snipcli::model::SnippetValidator::kMaxTitleLength, 'a');
        snipcli::model::SnippetValidator::validateTitle(maxTitle);
    });

    // ---------------------------------------------------------
    // Invariant 4: Title Boundary - Exceeding 100 Characters (101 chars)
    // ---------------------------------------------------------
    assertThrowsValidationException("Title exceeds max length (101 characters)", []() {
        std::string longTitle(snipcli::model::SnippetValidator::kMaxTitleLength + 1, 'a');
        snipcli::model::SnippetValidator::validateTitle(longTitle);
    });

    // ---------------------------------------------------------
    // Invariant 5: Title Character Constraints (Illegal Characters)
    // ---------------------------------------------------------
    assertThrowsValidationException("Title with special character @", []() {
        snipcli::model::SnippetValidator::validateTitle("Invalid@Title");
    });
    assertThrowsValidationException("Title with special character $", []() {
        snipcli::model::SnippetValidator::validateTitle("Invalid$Title");
    });
    assertThrowsValidationException("Title with special character !", []() {
        snipcli::model::SnippetValidator::validateTitle("Invalid!Title");
    });
    assertThrowsValidationException("Title with newline character", []() {
        snipcli::model::SnippetValidator::validateTitle("Invalid\nTitle");
    });

    // ---------------------------------------------------------
    // Invariant 6: Title Character Constraints (Allowed Characters)
    // ---------------------------------------------------------
    assertNoThrow("Title with alphanumeric, spaces, dashes, and underscores", []() {
        snipcli::model::SnippetValidator::validateTitle("Valid-Title_with Numbers 123");
    });

    // ---------------------------------------------------------
    // Invariant 7: Language Validation - Standard & Special Characters
    // ---------------------------------------------------------
    assertNoThrow("Valid language formats (c++, c#, python3)", []() {
        snipcli::model::SnippetValidator::validateLanguage("cpp");
        snipcli::model::SnippetValidator::validateLanguage("c++");
        snipcli::model::SnippetValidator::validateLanguage("c#");
        snipcli::model::SnippetValidator::validateLanguage("python3");
    });

    // ---------------------------------------------------------
    // Invariant 8: Language Validation - Empty & Disallowed Characters
    // ---------------------------------------------------------
    assertThrowsValidationException("Empty language", []() {
        snipcli::model::Snippet s(1, "ValidTitle", "", "int x = 0;", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });
    assertThrowsValidationException("Invalid language format with percent sign", []() {
        snipcli::model::SnippetValidator::validateLanguage("c%lang");
    });
    assertThrowsValidationException("Invalid language format with dash", []() {
        snipcli::model::SnippetValidator::validateLanguage("c-lang");
    });

    // ---------------------------------------------------------
    // Invariant 9: Code Content - Empty Code Rejection
    // ---------------------------------------------------------
    assertThrowsValidationException("Empty code content", []() {
        snipcli::model::Snippet s(1, "ValidTitle", "cpp", "", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // ---------------------------------------------------------
    // Invariant 10: Code Boundary - Exactly 1000 Lines (kMaxLinesAllowed)
    // ---------------------------------------------------------
    assertNoThrow("Code exactly 1000 lines boundary", []() {
        std::string exactMaxLines;
        for (size_t i = 0; i < snipcli::model::SnippetValidator::kMaxLinesAllowed; ++i) {
            exactMaxLines += "line\n";
        }
        snipcli::model::Snippet s(1, "BoundarySnippet", "cpp", exactMaxLines, {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // ---------------------------------------------------------
    // Invariant 11: Code Boundary - Exceeding 1000 Lines (1001 lines)
    // ---------------------------------------------------------
    assertThrowsValidationException("Code exceeds max lines (1001 lines)", []() {
        std::string tooManyLines;
        for (size_t i = 0; i <= snipcli::model::SnippetValidator::kMaxLinesAllowed; ++i) {
            tooManyLines += "line\n";
        }
        snipcli::model::Snippet s(1, "TooLargeSnippet", "cpp", tooManyLines, {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // ---------------------------------------------------------
    // Invariant 12: Tag Constraints - Empty Tags & Duplicate Tags
    // ---------------------------------------------------------
    assertThrowsValidationException("Empty tag string rejection", []() {
        snipcli::model::SnippetValidator::validateTags({"valid_tag", ""});
    });
    assertThrowsValidationException("Duplicate tag rejection", []() {
        snipcli::model::SnippetValidator::validateTags({"dp", "graph", "dp"});
    });
    assertNoThrow("Valid unique tags and empty tag list", []() {
        snipcli::model::SnippetValidator::validateTags({});
        snipcli::model::SnippetValidator::validateTags({"tree", "binary-tree", "dfs"});
    });

    std::cout << "[PASS] Validator tests passed. (12/12 Invariants Verified)" << std::endl;
}
