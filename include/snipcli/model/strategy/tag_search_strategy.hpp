#pragma once

#include <string>
#include <vector>

#include "snipcli/model/strategy/i_search_strategy.hpp"

namespace snipcli::model::strategy {

/**
 * @brief Concrete strategy for matching snippets by category tag.
 *
 * Implements case-insensitive exact tag matching. If the criteria is empty or
 * contains only whitespace, an empty vector is returned.
 */
class TagSearchStrategy : public ISearchStrategy {
public:
    /**
     * @brief Default constructor.
     */
    TagSearchStrategy() = default;

    /**
     * @brief Filters snippets possessing a tag matching the given criteria.
     *
     * Performs case-insensitive comparison against all tags in each candidate snippet.
     *
     * @param snippets Source collection of snippets to filter.
     * @param criteria The target tag name to search for.
     * @return std::vector<Snippet> Snippets containing the specified tag.
     */
    [[nodiscard]] std::vector<Snippet> filter(const std::vector<Snippet>& snippets,
                                              const std::string& criteria) const override;
};

}  // namespace snipcli::model::strategy
