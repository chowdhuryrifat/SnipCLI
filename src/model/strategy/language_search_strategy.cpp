#include "snipcli/model/strategy/language_search_strategy.hpp"

#include <algorithm>
#include <cctype>

namespace snipcli::model::strategy {

namespace {

std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return str;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

}  // namespace

std::vector<Snippet> LanguageSearchStrategy::filter(const std::vector<Snippet>& snippets,
                                                    const std::string& criteria) const {
    std::string trimmedCriteria = trim(criteria);
    if (trimmedCriteria.empty()) {
        return {};
    }

    const std::string lowerCriteria = toLower(trimmedCriteria);
    std::vector<Snippet> matches;

    for (const auto& snippet : snippets) {
        if (toLower(snippet.getLanguage()) == lowerCriteria) {
            matches.push_back(snippet);
        }
    }

    return matches;
}

}  // namespace snipcli::model::strategy
