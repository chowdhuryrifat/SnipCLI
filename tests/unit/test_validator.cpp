#include <iostream>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/model/snippet_validator.hpp"

void runValidatorTests() {
    std::cout << "[TEST] Running Validator tests..." << std::endl;
    // Test 1: Valid
    snipcli::model::Snippet snippet(1, "ValidTitle", "cpp", "int x = 10;", {"test"}, "", "");
    try {
        snipcli::model::SnippetValidator::validate(snippet);
    } catch (...) {
        std::cerr << "Valid snippet failed validation!" << std::endl;
        std::exit(1);
    }

    // Test 2: Empty Title
    bool caughtEmpty = false;
    snipcli::model::Snippet emptyTitle(1, "", "cpp", "int x = 10;", {}, "", "");
    try {
        snipcli::model::SnippetValidator::validate(emptyTitle);
    } catch (const snipcli::common::ValidationException&) {
        caughtEmpty = true;
    }
    if (!caughtEmpty) {
        std::cerr << "Empty title failed to throw ValidationException!" << std::endl;
        std::exit(1);
    }

    // Test 3: Duplicate Tags
    bool caughtDup = false;
    snipcli::model::Snippet dupTags(1, "DupTitle", "cpp", "int x = 10;", {"dp", "dp"}, "", "");
    try {
        snipcli::model::SnippetValidator::validate(dupTags);
    } catch (const snipcli::common::ValidationException&) {
        caughtDup = true;
    }
    if (!caughtDup) {
        std::cerr << "Duplicate tags failed to throw ValidationException!" << std::endl;
        std::exit(1);
    }

    std::cout << "[PASS] Validator tests passed." << std::endl;
}
