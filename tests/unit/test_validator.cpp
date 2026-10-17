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

}  // namespace

void runValidatorTests() {
    std::cout << "[TEST] Running Validator tests..." << std::endl;

    // Test 1: Valid snippet with standard characters and tags
    snipcli::model::Snippet valid(1, "Dijkstra_Shortest-Path v2", "c++",
                                  "int n = 10;\nvector<int> d;\n", {"graph", "sp"}, "", "");
    try {
        snipcli::model::SnippetValidator::validate(valid);
    } catch (...) {
        std::cerr << "[FAIL] Valid snippet unexpectedly failed validation!" << std::endl;
        std::exit(1);
    }

    // Test 2: Empty title
    assertThrowsValidationException("Empty title", []() {
        snipcli::model::Snippet s(1, "", "cpp", "int x = 0;", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // Test 3: Title exceeding max length (101 chars)
    assertThrowsValidationException("Title exceeds max length", []() {
        std::string longTitle(snipcli::model::SnippetValidator::kMaxTitleLength + 1, 'a');
        snipcli::model::SnippetValidator::validateTitle(longTitle);
    });

    // Test 4: Title with invalid special characters (@, $, !)
    assertThrowsValidationException("Title with special characters", []() {
        snipcli::model::SnippetValidator::validateTitle("Invalid@Title$");
    });

    // Test 5: Empty language
    assertThrowsValidationException("Empty language", []() {
        snipcli::model::Snippet s(1, "ValidTitle", "", "int x = 0;", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // Test 6: Invalid language format characters
    assertThrowsValidationException("Invalid language format", []() {
        snipcli::model::SnippetValidator::validateLanguage("c%lang");
    });

    // Test 7: Empty code content
    assertThrowsValidationException("Empty code content", []() {
        snipcli::model::Snippet s(1, "ValidTitle", "cpp", "", {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // Test 8: Code exceeding max line count (1001 lines)
    assertThrowsValidationException("Code exceeds max lines", []() {
        std::string manyLines;
        for (size_t i = 0; i <= snipcli::model::SnippetValidator::kMaxLinesAllowed; ++i) {
            manyLines += "line\n";
        }
        snipcli::model::Snippet s(1, "LargeSnippet", "cpp", manyLines, {}, "", "");
        snipcli::model::SnippetValidator::validate(s);
    });

    // Test 9: Empty tag string
    assertThrowsValidationException("Empty tag string", []() {
        snipcli::model::SnippetValidator::validateTags({"valid_tag", ""});
    });

    // Test 10: Duplicate tags
    assertThrowsValidationException("Duplicate tags", []() {
        snipcli::model::SnippetValidator::validateTags({"dp", "graph", "dp"});
    });

    std::cout << "[PASS] Validator tests passed. (10/10 Invariants Verified)" << std::endl;
}
