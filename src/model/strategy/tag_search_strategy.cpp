#include "snipcli/model/strategy/tag_search_strategy.hpp"

namespace snipcli::model::strategy {

std::vector<Snippet> TagSearchStrategy::filter(const std::vector<Snippet>& snippets,
                                               const std::string& criteria) const {
    std::vector<Snippet> matches;
    for (const auto& snippet : snippets) {
        if (snippet.hasTag(criteria)) {
            matches.push_back(snippet);
        }
    }
    return matches;
}

}  // namespace snipcli::model::strategy
