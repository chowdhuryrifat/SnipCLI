#pragma once

#include <string>
#include <vector>

#include "snipcli/model/strategy/i_search_strategy.hpp"

namespace snipcli::model::strategy {

/**
 * @brief Concrete strategy for matching snippets by title substring.
 *
 * Implements case-insensitive substring searching across snippet titles.
 * If the criteria is empty or contains only whitespace, an empty vector is returned.
 */
class TitleSearchStrategy : public ISearchStrategy {
public:
    /**
     * @brief Default constructor.
     */
    TitleSearchStrategy() = default;

    /**
     * @brief Filters snippets whose title contains the query substring.
     *
     * Performs case-insensitive substring matching against snippet titles.
     *
     * @param snippets Source collection of snippets to filter.
     * @param criteria Substring to search for within titles.
     * @return std::vector<Snippet> Snippets whose title contains the criteria.
     */
    [[nodiscard]] std::vector<Snippet> filter(const std::vector<Snippet>& snippets,
                                              const std::string& criteria) const override;
};

}  // namespace snipcli::model::strategy
