#include <iostream>

#include "snipcli/model/strategy/tag_search_strategy.hpp"
#include "snipcli/model/strategy/title_search_strategy.hpp"

void runStrategyTests() {
    std::cout << "[TEST] Running Strategy Pattern tests..." << std::endl;
    std::vector<snipcli::model::Snippet> list = {
        {1, "DijkstraAlgorithm", "cpp", "...", {"graph", "sp"}, "", ""},
        {2, "FloydWarshall", "cpp", "...", {"graph", "all_pairs"}, "", ""},
        {3, "KnapsackDP", "cpp", "...", {"dp"}, "", ""}};

    snipcli::model::strategy::TagSearchStrategy tagStrategy;
    auto graphMatches = tagStrategy.filter(list, "graph");
    if (graphMatches.size() != 2) {
        std::cerr << "TagSearchStrategy failed to match 2 items!" << std::endl;
        std::exit(1);
    }

    auto dpMatches = tagStrategy.filter(list, "dp");
    if (dpMatches.size() != 1 || dpMatches[0].getTitle() != "KnapsackDP") {
        std::cerr << "TagSearchStrategy failed to match KnapsackDP!" << std::endl;
        std::exit(1);
    }

    snipcli::model::strategy::TitleSearchStrategy titleStrategy;
    auto dijkstraMatches = titleStrategy.filter(list, "dijkstra");
    if (dijkstraMatches.size() != 1 || dijkstraMatches[0].getId() != 1) {
        std::cerr << "TitleSearchStrategy failed on 'dijkstra'!" << std::endl;
        std::exit(1);
    }

    auto algorithmMatches = titleStrategy.filter(list, "algorithm");
    if (algorithmMatches.size() != 1) {
        std::cerr << "TitleSearchStrategy failed on 'algorithm'!" << std::endl;
        std::exit(1);
    }

    std::cout << "[PASS] Strategy Pattern tests passed." << std::endl;
}
