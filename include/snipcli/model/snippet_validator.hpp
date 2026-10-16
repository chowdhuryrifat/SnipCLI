#pragma once

#include <string>

#include "snipcli/model/snippet.hpp"

namespace snipcli::model {

/**
 * @brief Validates snippet business constraints according to SRP.
 */
class SnippetValidator {
public:
    static constexpr size_t kMaxTitleLength = 100;
    static constexpr size_t kMaxLinesAllowed = 1000;

    /**
     * @brief Validates a snippet against core business rules.
     * @param snippet The snippet candidate to validate.
     * @throws ValidationException if any invariant is violated.
     */
    static void validate(const Snippet& snippet);

    /**
     * @brief Validates title syntax.
     * @param title The candidate title.
     * @throws ValidationException if title is invalid.
     */
    static void validateTitle(const std::string& title);

    /**
     * @brief Validates language syntax.
     * @param language The candidate language identifier.
     * @throws ValidationException if language is invalid.
     */
    static void validateLanguage(const std::string& language);
};

}  // namespace snipcli::model
