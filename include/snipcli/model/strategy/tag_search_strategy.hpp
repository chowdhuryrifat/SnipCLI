#pragma once

#include "snipcli/model/strategy/i_search_strategy.hpp"

namespace snipcli::model::strategy {

/**
 * @brief Concrete strategy for matching snippets by category tag.
 */
class TagSearchStrategy : public ISearchStrategy {
public:
    [[nodiscard]] std::vector<Snippet> filter(
        const std::vector<Snippet>& snippets, 
        const std::string& criteria) const override;
};

} // namespace snipcli::model::strategy
