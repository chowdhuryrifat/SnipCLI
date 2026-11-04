#include "snipcli/model/strategy/tag_search_strategy.hpp"

#include <algorithm>

#include "snipcli/common/string_utils.hpp"

namespace snipcli::model::strategy {

std::vector<Snippet> TagSearchStrategy::filter(const std::vector<Snippet>& snippets,
                                               const std::string& criteria) const {
    const std::string_view needle = common::trim(criteria);
    if (needle.empty()) {
        return {};
    }

    std::vector<Snippet> matches;
    for (const auto& snippet : snippets) {
        const auto& tags = snippet.getTags();
        if (std::any_of(tags.begin(), tags.end(), [needle](const std::string& tag) {
                return common::equalsIgnoreCase(tag, needle);
            })) {
            matches.push_back(snippet);
        }
    }
    return matches;
}

}  // namespace snipcli::model::strategy
