#pragma once

#include "snipcli/model/snippet.hpp"
#include <string>
#include <vector>

namespace snipcli::model::strategy {

/**
 * @brief Strategy Pattern (GoF): Behavioral interface for snippet searching and filtering.
 */
class ISearchStrategy {
public:
    virtual ~ISearchStrategy() = default;

    /**
     * @brief Filters a candidate list of snippets against specific criteria.
     * @param snippets Source collection of snippets.
     * @param criteria Query parameter (tag, substring, pattern).
     * @return std::vector<Snippet> Filtered result set.
     */
    [[nodiscard]] virtual std::vector<Snippet> filter(
        const std::vector<Snippet>& snippets, 
        const std::string& criteria) const = 0;
};

} // namespace snipcli::model::strategy
