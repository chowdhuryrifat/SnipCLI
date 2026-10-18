#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"

namespace snipcli::model {

/**
 * @brief Validates snippet business constraints according to the Single Responsibility Principle
 * (SRP).
 *
 * Exclusively responsible for enforcing domain data integrity and business rules before
 * snippet persistence. Quarantines all domain validation logic away from CLI argument
 * parsers and storage repositories, preventing invalid or corrupt data from entering the database.
 */
class SnippetValidator {
public:
    /// Maximum allowed character length for a snippet title.
    static constexpr size_t kMaxTitleLength = 100;

    /// Maximum permitted number of lines in a snippet source code body.
    static constexpr size_t kMaxLinesAllowed = 1000;

    /**
     * @brief Validates a snippet against all core business constraints.
     * @param snippet The snippet candidate entity to validate.
     * @throws common::ValidationException If title, language, code, or tags violate invariants.
     */
    static void validate(const Snippet& snippet);

    /**
     * @brief Validates snippet title syntax and length constraints.
     *
     * Invariants enforced:
     * - Title must not be empty.
     * - Title length must not exceed kMaxTitleLength (100 characters).
     * - Title characters must be alphanumeric, dashes, underscores, or spaces.
     *
     * @param title The candidate title string.
     * @throws common::ValidationException If title violates any invariant.
     */
    static void validateTitle(const std::string& title);

    /**
     * @brief Validates programming language syntax format.
     *
     * Invariants enforced:
     * - Language identifier must not be empty.
     * - Language characters must be alphanumeric, '+', or '#' (e.g., "cpp", "c++", "c#",
     * "python3").
     *
     * @param language The candidate language identifier.
     * @throws common::ValidationException If language violates any invariant.
     */
    static void validateLanguage(const std::string& language);

    /**
     * @brief Validates snippet source code payload and line count limits.
     *
     * Invariants enforced:
     * - Source code content must not be empty.
     * - Code line count must not exceed kMaxLinesAllowed (1000 lines).
     *
     * @param code The source code string payload.
     * @param lineCount The total line count of the source code.
     * @throws common::ValidationException If code is empty or exceeds line count limit.
     */
    static void validateCode(const std::string& code, size_t lineCount);

    /**
     * @brief Validates category tags for non-emptiness and uniqueness.
     *
     * Invariants enforced:
     * - Tags must not contain empty strings.
     * - Tags within the collection must be unique (no duplicate tags allowed).
     *
     * @param tags Collection of category tags.
     * @throws common::ValidationException If empty or duplicate tags are detected.
     */
    static void validateTags(const std::vector<std::string>& tags);
};

}  // namespace snipcli::model
