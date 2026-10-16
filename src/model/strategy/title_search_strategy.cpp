#include "snipcli/model/strategy/title_search_strategy.hpp"
#include <algorithm>

namespace snipcli::model::strategy {

namespace {
std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}
} // namespace

std::vector<Snippet> TitleSearchStrategy::filter(
    const std::vector<Snippet>& snippets, 
    const std::string& criteria) const {
    
    std::vector<Snippet> matches;
    const std::string lowerCriteria = toLower(criteria);

    for (const auto& snippet : snippets) {
        const std::string lowerTitle = toLower(snippet.getTitle());
        if (lowerTitle.find(lowerCriteria) != std::string::npos) {
            matches.push_back(snippet);
        }
    }
    return matches;
}

} // namespace snipcli::model::strategy
