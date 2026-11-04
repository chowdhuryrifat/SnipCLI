#include "snipcli/model/strategy/language_search_strategy.hpp"

#include "snipcli/common/string_utils.hpp"

namespace snipcli::model::strategy {

std::vector<Snippet> LanguageSearchStrategy::filter(const std::vector<Snippet>& snippets,
                                                    const std::string& criteria) const {
    const std::string_view needle = common::trim(criteria);
    if (needle.empty()) {
        return {};
    }

    std::vector<Snippet> matches;
    for (const auto& snippet : snippets) {
        if (common::equalsIgnoreCase(snippet.getLanguage(), needle)) {
            matches.push_back(snippet);
        }
    }
    return matches;
}

}  // namespace snipcli::model::strategy
