#pragma once

#include <string>
#include <vector>

#include "snipcli/model/strategy/i_search_strategy.hpp"

namespace snipcli::model::strategy {

/**
 * @brief Concrete strategy for matching snippets by programming language.
 *
 * Implements case-insensitive matching against snippet language identifiers
 * (e.g., "cpp", "python").
 * If the criteria is empty or contains only whitespace, an empty vector is returned.
 */
class LanguageSearchStrategy : public ISearchStrategy {
public:
    /**
     * @brief Default constructor.
     */
    LanguageSearchStrategy() = default;

    /**
     * @brief Filters snippets whose programming language matches the given criteria.
     *
     * Performs case-insensitive comparison against snippet language attributes.
     *
     * @param snippets Source collection of snippets to filter.
     * @param criteria The target programming language name.
     * @return std::vector<Snippet> Snippets matching the target language.
     */
    [[nodiscard]] std::vector<Snippet> filter(const std::vector<Snippet>& snippets,
                                              const std::string& criteria) const override;
};

}  // namespace snipcli::model::strategy
