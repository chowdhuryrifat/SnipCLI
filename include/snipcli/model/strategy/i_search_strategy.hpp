#pragma once

#include <string>
#include <vector>

#include "snipcli/model/snippet.hpp"

namespace snipcli::model::strategy {

/**
 * @brief Strategy Pattern (GoF): Behavioral interface for snippet searching and filtering.
 *
 * Defines the contract for interchangeable search algorithms. Conforms to the Open/Closed
 * Principle (OCP) by enabling new search and filtering mechanisms (e.g., tag matching,
 * substring search, regex matching, fuzzy matching) to be introduced without modifying
 * consuming commands or business services.
 */
class ISearchStrategy {
public:
    virtual ~ISearchStrategy() = default;

    /**
     * @brief Filters a candidate collection of snippets against specific query criteria.
     *
     * Implementations must remain stateless, thread-safe, and side-effect free.
     *
     * @param snippets The input collection of snippets to be filtered.
     * @param criteria The search parameter string (e.g., tag name, title substring).
     * @return std::vector<Snippet> Collection of snippets matching the criteria.
     */
    [[nodiscard]] virtual std::vector<Snippet> filter(const std::vector<Snippet>& snippets,
                                                      const std::string& criteria) const = 0;
};

}  // namespace snipcli::model::strategy
