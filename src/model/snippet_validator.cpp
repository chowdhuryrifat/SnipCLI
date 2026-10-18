#include "snipcli/model/snippet_validator.hpp"

#include <algorithm>
#include <unordered_set>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::model {

void SnippetValidator::validate(const Snippet& snippet) {
    validateTitle(snippet.getTitle());
    validateLanguage(snippet.getLanguage());
    validateCode(snippet.getCode(), snippet.getLineCount());
    validateTags(snippet.getTags());
}

void SnippetValidator::validateTitle(const std::string& title) {
    if (title.empty()) {
        throw common::ValidationException("Snippet title cannot be empty.");
    }
    if (title.length() > kMaxTitleLength) {
        throw common::ValidationException("Snippet title exceeds maximum length of " +
                                          std::to_string(kMaxTitleLength) + " characters.");
    }
    for (char c : title) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != ' ') {
            throw common::ValidationException(
                "Snippet title contains invalid characters. Only alphanumeric, dashes, and "
                "underscores allowed.");
        }
    }
}

void SnippetValidator::validateLanguage(const std::string& language) {
    if (language.empty()) {
        throw common::ValidationException("Snippet language must be specified.");
    }
    for (char c : language) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '+' && c != '#') {
            throw common::ValidationException("Invalid language format: '" + language + "'.");
        }
    }
}

void SnippetValidator::validateCode(const std::string& code, size_t lineCount) {
    if (code.empty()) {
        throw common::ValidationException("Snippet code content cannot be empty.");
    }

    if (lineCount > kMaxLinesAllowed) {
        throw common::ValidationException("Snippet exceeds maximum permitted line count of " +
                                          std::to_string(kMaxLinesAllowed) + " lines.");
    }
}

void SnippetValidator::validateTags(const std::vector<std::string>& tags) {
    std::unordered_set<std::string> seenTags;
    for (const auto& tag : tags) {
        if (tag.empty()) {
            throw common::ValidationException("Tags cannot be empty strings.");
        }
        if (!seenTags.insert(tag).second) {
            throw common::ValidationException("Duplicate tag detected: '" + tag + "'.");
        }
    }
}

}  // namespace snipcli::model
