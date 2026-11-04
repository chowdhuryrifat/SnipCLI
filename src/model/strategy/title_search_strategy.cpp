#include "snipcli/model/strategy/title_search_strategy.hpp"

#include "snipcli/common/string_utils.hpp"

namespace snipcli::model::strategy {

std::vector<Snippet> TitleSearchStrategy::filter(const std::vector<Snippet>& snippets,
                                                 const std::string& criteria) const {
    const std::string_view trimmed = common::trim(criteria);
    if (trimmed.empty()) {
        return {};
    }

    const std::string needle = common::toLower(trimmed);
    std::vector<Snippet> matches;
    for (const auto& snippet : snippets) {
        if (common::toLower(snippet.getTitle()).find(needle) != std::string::npos) {
            matches.push_back(snippet);
        }
    }
    return matches;
}

}  // namespace snipcli::model::strategy
