#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "snipcli/model/strategy/i_search_strategy.hpp"
#include "snipcli/model/strategy/language_search_strategy.hpp"
#include "snipcli/model/strategy/tag_search_strategy.hpp"
#include "snipcli/model/strategy/title_search_strategy.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] Strategy Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runStrategyTests() {
    std::cout << "[TEST] Running Strategy Pattern tests..." << std::endl;

    const std::vector<snipcli::model::Snippet> sampleSnippets = {
        {1, "DijkstraAlgorithm", "cpp", "vector<int> dist;", {"graph", "sp"}, "", ""},
        {2, "FloydWarshall", "cpp", "int d[N][N];", {"graph", "all_pairs"}, "", ""},
        {3, "KnapsackDP", "cpp", "int dp[W];", {"dp"}, "", ""},
        {4, "BinarySearch", "python", "l, r = 0, n - 1", {"search", "binary"}, "", ""},
        {5, "SegmentTree", "cpp", "void build();", {}, "", ""}};

    // ---------------------------------------------------------
    // Scenario 1: TagSearchStrategy - Exact Tag Matching
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TagSearchStrategy tagStrategy;
        auto matches = tagStrategy.filter(sampleSnippets, "graph");
        assertTrue(matches.size() == 2, "Tag 'graph' must match exactly 2 snippets");
        assertTrue(matches[0].getId() == 1 && matches[1].getId() == 2,
                   "Matched snippets must be DijkstraAlgorithm (1) and FloydWarshall (2)");
    }

    // ---------------------------------------------------------
    // Scenario 2: TagSearchStrategy - Case-Insensitive Matching
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TagSearchStrategy tagStrategy;
        auto upperGraph = tagStrategy.filter(sampleSnippets, "GRAPH");
        assertTrue(upperGraph.size() == 2, "Uppercase query 'GRAPH' must match lowercase tags");

        auto mixedDp = tagStrategy.filter(sampleSnippets, "Dp");
        assertTrue(mixedDp.size() == 1 && mixedDp[0].getTitle() == "KnapsackDP",
                   "Mixed-case query 'Dp' must match 'dp'");
    }

    // ---------------------------------------------------------
    // Scenario 3: TagSearchStrategy - Whitespace Trimming
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TagSearchStrategy tagStrategy;
        auto padded = tagStrategy.filter(sampleSnippets, "  sp  ");
        assertTrue(padded.size() == 1 && padded[0].getId() == 1,
                   "Padded tag query '  sp  ' must match 'sp'");
    }

    // ---------------------------------------------------------
    // Scenario 4: TagSearchStrategy - Non-Matching & Empty Tag Snippets
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TagSearchStrategy tagStrategy;
        auto noMatch = tagStrategy.filter(sampleSnippets, "geometry");
        assertTrue(noMatch.empty(), "Unmatched tag 'geometry' must return empty result");

        // Snippet 5 has empty tags and must not crash or match
        auto allTags = tagStrategy.filter(sampleSnippets, "search");
        assertTrue(allTags.size() == 1 && allTags[0].getId() == 4,
                   "Snippet with empty tags ignored safely");
    }

    // ---------------------------------------------------------
    // Scenario 5: TagSearchStrategy - Empty & Blank Criteria
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TagSearchStrategy tagStrategy;
        assertTrue(tagStrategy.filter(sampleSnippets, "").empty(),
                   "Empty tag criteria must return empty vector");
        assertTrue(tagStrategy.filter(sampleSnippets, "   ").empty(),
                   "Whitespace-only tag criteria must return empty vector");
    }

    // ---------------------------------------------------------
    // Scenario 6: TitleSearchStrategy - Exact & Case-Insensitive Match
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TitleSearchStrategy titleStrategy;
        auto exact = titleStrategy.filter(sampleSnippets, "DijkstraAlgorithm");
        assertTrue(exact.size() == 1 && exact[0].getId() == 1,
                   "Exact title match must find DijkstraAlgorithm");

        auto lower = titleStrategy.filter(sampleSnippets, "dijkstraalgorithm");
        assertTrue(lower.size() == 1 && lower[0].getId() == 1,
                   "Lowercase query must match mixed-case title");

        auto upper = titleStrategy.filter(sampleSnippets, "FLOYDWARSHALL");
        assertTrue(upper.size() == 1 && upper[0].getId() == 2,
                   "Uppercase query must match mixed-case title");
    }

    // ---------------------------------------------------------
    // Scenario 7: TitleSearchStrategy - Substring Matching
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TitleSearchStrategy titleStrategy;
        auto sub1 = titleStrategy.filter(sampleSnippets, "Warshall");
        assertTrue(sub1.size() == 1 && sub1[0].getTitle() == "FloydWarshall",
                   "Substring 'Warshall' must match FloydWarshall");

        auto sub2 = titleStrategy.filter(sampleSnippets, "tree");
        assertTrue(sub2.size() == 1 && sub2[0].getTitle() == "SegmentTree",
                   "Substring 'tree' must match SegmentTree");

        auto sub3 = titleStrategy.filter(sampleSnippets, "Algorithm");
        assertTrue(sub3.size() == 1 && sub3[0].getId() == 1,
                   "Substring 'Algorithm' must match DijkstraAlgorithm");
    }

    // ---------------------------------------------------------
    // Scenario 8: TitleSearchStrategy - Non-Matching & Blank Criteria
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::TitleSearchStrategy titleStrategy;
        auto missing = titleStrategy.filter(sampleSnippets, "NonExistentSubstring");
        assertTrue(missing.empty(), "Non-matching substring must return empty result");

        assertTrue(titleStrategy.filter(sampleSnippets, "").empty(),
                   "Empty title criteria must return empty vector");
        assertTrue(titleStrategy.filter(sampleSnippets, "   ").empty(),
                   "Whitespace-only title criteria must return empty vector");
    }

    // ---------------------------------------------------------
    // Scenario 9: LanguageSearchStrategy - Language Filtering
    // ---------------------------------------------------------
    {
        snipcli::model::strategy::LanguageSearchStrategy langStrategy;
        auto cppMatches = langStrategy.filter(sampleSnippets, "cpp");
        assertTrue(cppMatches.size() == 4, "Language 'cpp' must match 4 snippets");

        auto pythonMatches = langStrategy.filter(sampleSnippets, "PYTHON");
        assertTrue(pythonMatches.size() == 1 && pythonMatches[0].getTitle() == "BinarySearch",
                   "Case-insensitive query 'PYTHON' must match 'python'");

        auto javaMatches = langStrategy.filter(sampleSnippets, "java");
        assertTrue(javaMatches.empty(), "Unrepresented language 'java' must return empty vector");

        assertTrue(langStrategy.filter(sampleSnippets, "").empty(),
                   "Empty language criteria must return empty vector");
        assertTrue(langStrategy.filter(sampleSnippets, "  \t ").empty(),
                   "Blank language criteria must return empty vector");
    }

    // ---------------------------------------------------------
    // Scenario 10: Polymorphism, Statelessness & Empty Candidate List
    // ---------------------------------------------------------
    {
        std::vector<std::unique_ptr<snipcli::model::strategy::ISearchStrategy>> strategies;
        strategies.push_back(std::make_unique<snipcli::model::strategy::TagSearchStrategy>());
        strategies.push_back(std::make_unique<snipcli::model::strategy::TitleSearchStrategy>());
        strategies.push_back(std::make_unique<snipcli::model::strategy::LanguageSearchStrategy>());

        for (const auto& strat : strategies) {
            // Polymorphic call on empty snippet collection
            auto emptyRes = strat->filter({}, "test");
            assertTrue(emptyRes.empty(), "Strategy must handle empty candidate list safely");

            // Verify idempotency / statelessness
            auto res1 = strat->filter(sampleSnippets, "cpp");
            auto res2 = strat->filter(sampleSnippets, "cpp");
            assertTrue(res1.size() == res2.size(),
                       "Strategy must remain stateless across invocations");
        }
    }

    std::cout << "[PASS] Strategy Pattern tests passed. (10/10 Scenarios Verified)" << std::endl;
}
