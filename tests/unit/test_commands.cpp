#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/commands/add_command.hpp"
#include "snipcli/controller/commands/delete_command.hpp"
#include "snipcli/controller/commands/get_command.hpp"
#include "snipcli/controller/commands/list_command.hpp"
#include "snipcli/controller/commands/search_command.hpp"
#include "snipcli/controller/commands/stats_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/model/snippet.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] Command Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

// In-Memory Repository Mock conforming to ISP (ISnippetReader & ISnippetWriter)
class MockRepository : public snipcli::model::ISnippetRepository {
public:
    uint64_t save(snipcli::model::Snippet& snippet) override {
        uint64_t id = m_nextId++;
        snipcli::model::Snippet saved(id, snippet.getTitle(), snippet.getLanguage(),
                                      snippet.getCode(), snippet.getTags(), "2026-10-19T00:00:00Z",
                                      "2026-10-19T00:00:00Z");
        m_snippets.push_back(saved);
        return id;
    }

    std::optional<snipcli::model::Snippet> getById(uint64_t id) const override {
        for (const auto& s : m_snippets) {
            if (s.getId() == id) {
                return s;
            }
        }
        return std::nullopt;
    }

    std::optional<snipcli::model::Snippet> getByTitle(const std::string& title) const override {
        for (const auto& s : m_snippets) {
            if (s.getTitle() == title) {
                return s;
            }
        }
        return std::nullopt;
    }

    std::vector<snipcli::model::Snippet> getAll() const override { return m_snippets; }

    bool exists(const std::string& title) const override { return getByTitle(title).has_value(); }

    size_t count() const override { return m_snippets.size(); }

    bool removeById(uint64_t id) override {
        for (auto it = m_snippets.begin(); it != m_snippets.end(); ++it) {
            if (it->getId() == id) {
                m_snippets.erase(it);
                return true;
            }
        }
        return false;
    }

    bool removeByTitle(const std::string& title) override {
        for (auto it = m_snippets.begin(); it != m_snippets.end(); ++it) {
            if (it->getTitle() == title) {
                m_snippets.erase(it);
                return true;
            }
        }
        return false;
    }

    void addDirect(const snipcli::model::Snippet& s) { m_snippets.push_back(s); }

private:
    uint64_t m_nextId{1};
    std::vector<snipcli::model::Snippet> m_snippets;
};

// In-Memory View Mock conforming to ISnippetView for recording outputs
class MockView : public snipcli::view::ISnippetView {
public:
    void renderSnippet(const snipcli::model::Snippet& snippet) override {
        renderedSnippets.push_back(snippet);
    }

    void renderSnippetList(const std::vector<snipcli::model::Snippet>& snippets) override {
        renderedLists.push_back(snippets);
    }

    void renderMessage(const std::string& message) override { messages.push_back(message); }

    void renderSuccess(const std::string& message) override { successes.push_back(message); }

    void renderError(const std::string& message) override { errors.push_back(message); }

    void renderStats(
        size_t totalSnippets, size_t totalLines,
        const std::vector<std::pair<std::string, size_t>>& languageBreakdown) override {
        statsCalls++;
        lastTotalSnippets = totalSnippets;
        lastTotalLines = totalLines;
        lastBreakdown = languageBreakdown;
    }

    std::vector<snipcli::model::Snippet> renderedSnippets;
    std::vector<std::vector<snipcli::model::Snippet>> renderedLists;
    std::vector<std::string> messages;
    std::vector<std::string> successes;
    std::vector<std::string> errors;
    size_t statsCalls{0};
    size_t lastTotalSnippets{0};
    size_t lastTotalLines{0};
    std::vector<std::pair<std::string, size_t>> lastBreakdown;
};

}  // namespace

void runCommandTests() {
    std::cout << "[TEST] Running GoF Command Wiring tests (Add, List, Get, Delete, Search)..."
              << std::endl;

    // ---------------------------------------------------------
    // Scenario 1: AddCommand - Inline Code and Tag Parsing
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        auto args = snipcli::controller::CliParser::parse({"add", "--title", "Dijkstra", "--lang",
                                                           "cpp", "--tags", "graph,sp", "--code",
                                                           "vector<int> dist;"});

        snipcli::controller::commands::AddCommand cmd(args, repo, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "AddCommand with valid inline arguments must return 0");
        assertTrue(repo->count() == 1, "Repository must contain 1 snippet");
        assertTrue(repo->exists("Dijkstra"), "Snippet 'Dijkstra' must exist in repository");
        assertTrue(view->successes.size() == 1, "View must receive success notification");

        auto snippet = repo->getByTitle("Dijkstra");
        assertTrue(snippet.has_value(), "Snippet retrieval must succeed");
        assertTrue(snippet->getTags().size() == 2, "Snippet must possess 2 category tags");
        assertTrue(snippet->getTags()[0] == "graph" && snippet->getTags()[1] == "sp",
                   "Tags must be parsed and trimmed correctly");
    }

    // ---------------------------------------------------------
    // Scenario 2: AddCommand - Positional Title & Default Language
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        auto args = snipcli::controller::CliParser::parse(
            {"add", "BinarySearch", "--code", "int l = 0, r = n - 1;"});

        snipcli::controller::commands::AddCommand cmd(args, repo, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "AddCommand with positional title must return 0");
        auto s = repo->getByTitle("BinarySearch");
        assertTrue(s.has_value(), "Snippet with positional title must be saved");
        assertTrue(s->getLanguage() == "cpp", "Default language must be 'cpp'");
    }

    // ---------------------------------------------------------
    // Scenario 3: AddCommand - Reading Source Code from File (--file)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        const std::string tempFilePath = "test_temp_snippet.cpp";
        {
            std::ofstream tempFile(tempFilePath);
            tempFile << "void segmentTreeBuild() { int tree[4*N]; }\n";
        }

        auto args = snipcli::controller::CliParser::parse(
            {"add", "--title", "SegmentTree", "--lang", "cpp", "--file", tempFilePath});

        snipcli::controller::commands::AddCommand cmd(args, repo, repo, view);
        int code = cmd.execute();

        std::remove(tempFilePath.c_str());

        assertTrue(code == 0, "AddCommand with --file must succeed");
        auto s = repo->getByTitle("SegmentTree");
        assertTrue(s.has_value(), "Snippet from file must be persisted");
        assertTrue(s->getCode().find("segmentTreeBuild") != std::string::npos,
                   "File content must match persisted code");
    }

    // ---------------------------------------------------------
    // Scenario 4: AddCommand - Non-Existent File Error Handling
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        auto args = snipcli::controller::CliParser::parse(
            {"add", "--title", "MissingFileSnippet", "--file", "non_existent_file_xyz.cpp"});

        snipcli::controller::commands::AddCommand cmd(args, repo, repo, view);
        int code = cmd.execute();

        assertTrue(code != 0, "AddCommand with missing file must return non-zero exit code");
        assertTrue(view->errors.size() == 1, "Error view must receive failure notification");
        assertTrue(view->errors[0].find("Unable to read source code file") != std::string::npos,
                   "Error notification must describe file access failure");
    }

    // ---------------------------------------------------------
    // Scenario 5: AddCommand - Duplicate Title Rejection
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "FloydWarshall", "cpp", "int d[N][N];", {"graph"}, "", ""});

        auto args = snipcli::controller::CliParser::parse(
            {"add", "--title", "FloydWarshall", "--code", "int dist[N][N];"});

        snipcli::controller::commands::AddCommand cmd(args, repo, repo, view);
        int code = cmd.execute();

        assertTrue(code != 0, "AddCommand must reject duplicate title");
        assertTrue(view->errors.size() == 1, "Duplicate title error must be rendered");
        assertTrue(view->errors[0].find("already exists") != std::string::npos,
                   "Error message must specify snippet already exists");
    }

    // ---------------------------------------------------------
    // Scenario 6: AddCommand - Validation Invariant Rejection
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // Missing code
        auto args1 = snipcli::controller::CliParser::parse({"add", "--title", "NoCode"});
        snipcli::controller::commands::AddCommand cmd1(args1, repo, repo, view);
        assertTrue(cmd1.execute() != 0, "Snippet without code must fail validation");

        // Empty title
        auto args2 = snipcli::controller::CliParser::parse(
            {"add", "--title", "", "--code", "int dummy = 1;"});
        snipcli::controller::commands::AddCommand cmd2(args2, repo, repo, view);
        assertTrue(cmd2.execute() != 0, "Snippet with empty title must fail validation");
    }

    // ---------------------------------------------------------
    // Scenario 7: ListCommand - Unfiltered Listing
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "Alpha", "cpp", "int a;", {"math"}, "", ""});
        repo->addDirect({2, "Beta", "python", "b = 1", {"math"}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"list"});
        snipcli::controller::commands::ListCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "ListCommand must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList must be invoked once");
        assertTrue(view->renderedLists[0].size() == 2, "Unfiltered list must contain all snippets");
    }

    // ---------------------------------------------------------
    // Scenario 8: ListCommand - Filtering by Tag (GoF Strategy Wiring)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "Dijkstra", "cpp", "code1", {"graph", "sp"}, "", ""});
        repo->addDirect({2, "Knapsack", "cpp", "code2", {"dp"}, "", ""});
        repo->addDirect({3, "Kruskal", "cpp", "code3", {"graph", "mst"}, "", ""});

        // Case-insensitive query "GRAPH"
        auto args = snipcli::controller::CliParser::parse({"list", "--tag", "GRAPH"});
        snipcli::controller::commands::ListCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "ListCommand with --tag must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList must be invoked");
        assertTrue(view->renderedLists[0].size() == 2,
                   "Tag filtering must return exactly 2 graph snippets");
    }

    // ---------------------------------------------------------
    // Scenario 9: ListCommand - Filtering by Language (GoF Strategy Wiring)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "CppSnippet1", "cpp", "code1", {}, "", ""});
        repo->addDirect({2, "PySnippet", "python", "code2", {}, "", ""});
        repo->addDirect({3, "CppSnippet2", "cpp", "code3", {}, "", ""});

        // Case-insensitive query "PYTHON"
        auto args = snipcli::controller::CliParser::parse({"list", "--lang", "PYTHON"});
        snipcli::controller::commands::ListCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "ListCommand with --lang must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList must be invoked");
        assertTrue(view->renderedLists[0].size() == 1, "Language filter must return 1 snippet");
        assertTrue(view->renderedLists[0][0].getTitle() == "PySnippet",
                   "Language filter must match PySnippet");
    }

    // ---------------------------------------------------------
    // Scenario 10: ListCommand - Combined Tag and Language Filter
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "DijkstraCpp", "cpp", "code1", {"graph"}, "", ""});
        repo->addDirect({2, "DijkstraPy", "python", "code2", {"graph"}, "", ""});
        repo->addDirect({3, "KnapsackCpp", "cpp", "code3", {"dp"}, "", ""});

        auto args =
            snipcli::controller::CliParser::parse({"list", "--tag", "graph", "--lang", "cpp"});
        snipcli::controller::commands::ListCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "Combined filter must return 0");
        assertTrue(view->renderedLists[0].size() == 1, "Only DijkstraCpp matches both criteria");
        assertTrue(view->renderedLists[0][0].getTitle() == "DijkstraCpp",
                   "Matched snippet must be DijkstraCpp");
    }

    // ---------------------------------------------------------
    // Scenario 11: GetCommand - Retrieval by ID and Title
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({42, "TargetSnippet", "cpp", "target_code();", {"algo"}, "", ""});

        // Query by --id
        auto argsId = snipcli::controller::CliParser::parse({"get", "--id", "42"});
        snipcli::controller::commands::GetCommand cmdId(argsId, repo, view);
        assertTrue(cmdId.execute() == 0, "GetCommand by --id must succeed");
        assertTrue(view->renderedSnippets.size() == 1 && view->renderedSnippets[0].getId() == 42,
                   "Rendered snippet must match ID 42");

        // Query by --title
        auto argsTitle = snipcli::controller::CliParser::parse({"get", "--title", "TargetSnippet"});
        snipcli::controller::commands::GetCommand cmdTitle(argsTitle, repo, view);
        assertTrue(cmdTitle.execute() == 0, "GetCommand by --title must succeed");
        assertTrue(view->renderedSnippets.size() == 2, "Rendered snippet count must be 2");

        // Positional query
        auto argsPos = snipcli::controller::CliParser::parse({"get", "TargetSnippet"});
        snipcli::controller::commands::GetCommand cmdPos(argsPos, repo, view);
        assertTrue(cmdPos.execute() == 0, "GetCommand with positional title must succeed");
        assertTrue(view->renderedSnippets.size() == 3, "Rendered snippet count must be 3");
    }

    // ---------------------------------------------------------
    // Scenario 12: GetCommand - Code-Only Pipe Mode (--code-only)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "PureCodeSnippet", "cpp", "int pure_code_body = 100;", {}, "", ""});

        auto args = snipcli::controller::CliParser::parse(
            {"get", "--title", "PureCodeSnippet", "--code-only"});
        snipcli::controller::commands::GetCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "GetCommand with --code-only must succeed");
        assertTrue(view->renderedSnippets.empty(),
                   "renderSnippet must NOT be called in --code-only mode");
        assertTrue(view->messages.size() == 1,
                   "Pure code body must be routed via renderMessage to stream");
        assertTrue(view->messages[0] == "int pure_code_body = 100;",
                   "Emitted message must match exact code body");
    }

    // ---------------------------------------------------------
    // Scenario 13: GetCommand - Not Found & Invalid ID Format
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // Non-existent snippet
        auto argsMissing = snipcli::controller::CliParser::parse({"get", "--title", "NoSuchTitle"});
        snipcli::controller::commands::GetCommand cmdMissing(argsMissing, repo, view);
        assertTrue(cmdMissing.execute() != 0, "GetCommand for non-existent snippet must fail");

        // Invalid non-integer ID format
        auto argsBadId = snipcli::controller::CliParser::parse({"get", "--id", "not_a_number"});
        snipcli::controller::commands::GetCommand cmdBadId(argsBadId, repo, view);
        assertTrue(cmdBadId.execute() != 0, "GetCommand with invalid ID format must fail");

        // Missing lookup criteria
        auto argsNoArg = snipcli::controller::CliParser::parse({"get"});
        snipcli::controller::commands::GetCommand cmdNoArg(argsNoArg, repo, view);
        assertTrue(cmdNoArg.execute() != 0, "GetCommand without criteria must fail");
    }

    // ---------------------------------------------------------
    // Scenario 14: DeleteCommand - Deletion by --id
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({10, "ToDeleteById", "cpp", "code", {}, "", ""});
        assertTrue(repo->count() == 1, "Initial snippet count is 1");

        auto args = snipcli::controller::CliParser::parse({"delete", "--id", "10"});
        snipcli::controller::commands::DeleteCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "DeleteCommand by --id must return 0");
        assertTrue(repo->count() == 0, "Snippet must be removed from repository");
        assertTrue(view->successes.size() == 1, "Success notification rendered");
        assertTrue(view->successes[0].find("ID #10") != std::string::npos,
                   "Success message references ID #10");
    }

    // ---------------------------------------------------------
    // Scenario 15: DeleteCommand - Deletion by --title
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "ToDeleteByTitle", "cpp", "code", {}, "", ""});
        auto args = snipcli::controller::CliParser::parse({"delete", "--title", "ToDeleteByTitle"});
        snipcli::controller::commands::DeleteCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "DeleteCommand by --title must return 0");
        assertTrue(repo->count() == 0, "Snippet must be removed from repository");
        assertTrue(view->successes.size() == 1, "Success notification rendered");
    }

    // ---------------------------------------------------------
    // Scenario 16: DeleteCommand - Positional Identification (Numeric & Title)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({5, "NumericPositional", "cpp", "code", {}, "", ""});
        repo->addDirect({6, "StringPositional", "cpp", "code", {}, "", ""});

        // Numeric positional resolves to ID lookup
        auto argsNum = snipcli::controller::CliParser::parse({"delete", "5"});
        snipcli::controller::commands::DeleteCommand cmdNum(argsNum, repo, view);
        assertTrue(cmdNum.execute() == 0, "Positional numeric delete succeeds");
        assertTrue(repo->count() == 1, "One snippet remaining");

        // String positional resolves to Title lookup
        auto argsStr = snipcli::controller::CliParser::parse({"delete", "StringPositional"});
        snipcli::controller::commands::DeleteCommand cmdStr(argsStr, repo, view);
        assertTrue(cmdStr.execute() == 0, "Positional title delete succeeds");
        assertTrue(repo->count() == 0, "All snippets removed");
    }

    // ---------------------------------------------------------
    // Scenario 17: DeleteCommand - Not Found, Bad ID Format & Missing Args
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // Non-existent snippet
        auto argsMissing =
            snipcli::controller::CliParser::parse({"delete", "--title", "NonExistent"});
        snipcli::controller::commands::DeleteCommand cmdMissing(argsMissing, repo, view);
        assertTrue(cmdMissing.execute() != 0, "Delete non-existent snippet must fail");

        // Invalid non-integer ID format
        auto argsBadId = snipcli::controller::CliParser::parse({"delete", "--id", "bad_id"});
        snipcli::controller::commands::DeleteCommand cmdBadId(argsBadId, repo, view);
        assertTrue(cmdBadId.execute() != 0, "Delete with invalid ID format must fail");

        // Missing arguments
        auto argsNoArg = snipcli::controller::CliParser::parse({"delete"});
        snipcli::controller::commands::DeleteCommand cmdNoArg(argsNoArg, repo, view);
        assertTrue(cmdNoArg.execute() != 0, "Delete without criteria must fail");
    }

    // ---------------------------------------------------------
    // Scenario 18: SearchCommand - Search by Tag (--tag)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "Dijkstra", "cpp", "code", {"graph", "sp"}, "", ""});
        repo->addDirect({2, "FloydWarshall", "cpp", "code", {"graph", "all_pairs"}, "", ""});
        repo->addDirect({3, "Knapsack", "cpp", "code", {"dp"}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"search", "--tag", "graph"});
        snipcli::controller::commands::SearchCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "SearchCommand with --tag must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList called");
        assertTrue(view->renderedLists[0].size() == 2, "2 graph snippets returned");
    }

    // ---------------------------------------------------------
    // Scenario 19: SearchCommand - Search by Language (--lang)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "Dijkstra", "cpp", "code", {}, "", ""});
        repo->addDirect({2, "BinarySearch", "python", "code", {}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"search", "--lang", "PYTHON"});
        snipcli::controller::commands::SearchCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "SearchCommand with --lang must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList called");
        assertTrue(view->renderedLists[0].size() == 1, "1 python snippet returned");
        assertTrue(view->renderedLists[0][0].getTitle() == "BinarySearch",
                   "Result matches BinarySearch");
    }

    // ---------------------------------------------------------
    // Scenario 20: SearchCommand - Search by Title Substring (--title)
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "SegmentTreeRangeQuery", "cpp", "code", {}, "", ""});
        repo->addDirect({2, "BinarySearchTree", "cpp", "code", {}, "", ""});
        repo->addDirect({3, "Dijkstra", "cpp", "code", {}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"search", "--title", "tree"});
        snipcli::controller::commands::SearchCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "SearchCommand with --title must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList called");
        assertTrue(view->renderedLists[0].size() == 2,
                   "2 tree snippets matched (SegmentTree and BinarySearchTree)");
    }

    // ---------------------------------------------------------
    // Scenario 21: SearchCommand - Positional Query Substring Search
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        repo->addDirect({1, "DijkstraShortestPath", "cpp", "code", {}, "", ""});
        repo->addDirect({2, "KnapsackDP", "cpp", "code", {}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"search", "Shortest"});
        snipcli::controller::commands::SearchCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "Positional search must return 0");
        assertTrue(view->renderedLists.size() == 1, "renderSnippetList called");
        assertTrue(view->renderedLists[0].size() == 1, "1 match found for 'Shortest'");
        assertTrue(view->renderedLists[0][0].getTitle() == "DijkstraShortestPath",
                   "Match is DijkstraShortestPath");
    }

    // ---------------------------------------------------------
    // Scenario 22: SearchCommand - Missing Criteria & Empty Results
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // Missing criteria
        auto argsEmpty = snipcli::controller::CliParser::parse({"search"});
        snipcli::controller::commands::SearchCommand cmdEmpty(argsEmpty, repo, view);
        assertTrue(cmdEmpty.execute() != 0, "Search without criteria must fail");
        assertTrue(view->errors.size() == 1, "Error message rendered");

        // Non-matching query returns empty list with 0 exit code
        auto argsNoMatch =
            snipcli::controller::CliParser::parse({"search", "NonExistentString12345"});
        snipcli::controller::commands::SearchCommand cmdNoMatch(argsNoMatch, repo, view);
        assertTrue(cmdNoMatch.execute() == 0, "Search with no matches returns 0");
        assertTrue(view->renderedLists.size() == 1 && view->renderedLists[0].empty(),
                   "Empty list passed to renderSnippetList");
    }

    // ---------------------------------------------------------
    // Scenario 23: StatsCommand - Multi-Language Metrics & Line Totals
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // 2 cpp snippets (3 lines + 2 lines = 5 lines)
        // 1 python snippet (1 line)
        repo->addDirect({1, "Dijkstra", "cpp", "line1\nline2\nline3\n", {}, "", ""});
        repo->addDirect({2, "Floyd", "cpp", "line1\nline2\n", {}, "", ""});
        repo->addDirect({3, "QuickSort", "python", "x = 1\n", {}, "", ""});

        auto args = snipcli::controller::CliParser::parse({"stats"});
        snipcli::controller::commands::StatsCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "StatsCommand must return 0");
        assertTrue(view->statsCalls == 1, "renderStats must be invoked once");
        assertTrue(view->lastTotalSnippets == 3, "Total snippets must be 3");
        assertTrue(view->lastTotalLines == 6, "Total lines must be 6 (3 + 2 + 1)");
        assertTrue(view->lastBreakdown.size() == 2, "Breakdown must have 2 languages");
        assertTrue(view->lastBreakdown[0].first == "cpp" && view->lastBreakdown[0].second == 2,
                   "Most frequent language 'cpp' (2) must be first in sorted telemetry");
        assertTrue(view->lastBreakdown[1].first == "python" && view->lastBreakdown[1].second == 1,
                   "Second language 'python' (1) must be second in sorted telemetry");
    }

    // ---------------------------------------------------------
    // Scenario 24: StatsCommand - Empty Repository Telemetry
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        auto args = snipcli::controller::CliParser::parse({"stats"});
        snipcli::controller::commands::StatsCommand cmd(args, repo, view);
        int code = cmd.execute();

        assertTrue(code == 0, "StatsCommand on empty repository must succeed");
        assertTrue(view->statsCalls == 1, "renderStats must be invoked");
        assertTrue(view->lastTotalSnippets == 0, "Total snippets must be 0");
        assertTrue(view->lastTotalLines == 0, "Total lines must be 0");
        assertTrue(view->lastBreakdown.empty(), "Language breakdown must be empty");
    }

    // ---------------------------------------------------------
    // Scenario 25: Strict Positive ID Validation & Storage Quarantine
    // ---------------------------------------------------------
    {
        auto repo = std::make_shared<MockRepository>();
        auto view = std::make_shared<MockView>();

        // 1. Negative ID in GetCommand
        auto argsNegGet = snipcli::controller::CliParser::parse({"get", "--id=-5"});
        snipcli::controller::commands::GetCommand cmdNegGet(argsNegGet, repo, view);
        int codeNegGet = cmdNegGet.execute();
        assertTrue(codeNegGet != 0, "GetCommand with negative ID (--id=-5) must fail");
        assertTrue(!view->errors.empty(), "Error message must be rendered for negative ID");
        assertTrue(view->errors.back().find("Invalid snippet ID format") != std::string::npos,
                   "Error must indicate 'Invalid snippet ID format'");

        // 2. Negative ID with equals syntax in DeleteCommand
        auto argsNegDel = snipcli::controller::CliParser::parse({"delete", "--id=-5"});
        snipcli::controller::commands::DeleteCommand cmdNegDel(argsNegDel, repo, view);
        int codeNegDel = cmdNegDel.execute();
        assertTrue(codeNegDel != 0, "DeleteCommand with negative ID (--id=-5) must fail");
        assertTrue(view->errors.back().find("Invalid snippet ID format") != std::string::npos,
                   "Error must indicate 'Invalid snippet ID format'");

        // 3. Zero ID rejected as non-positive integer
        auto argsZeroGet = snipcli::controller::CliParser::parse({"get", "--id", "0"});
        snipcli::controller::commands::GetCommand cmdZeroGet(argsZeroGet, repo, view);
        assertTrue(cmdZeroGet.execute() != 0, "GetCommand with zero ID (--id 0) must fail");

        auto argsZeroDel = snipcli::controller::CliParser::parse({"delete", "--id", "0"});
        snipcli::controller::commands::DeleteCommand cmdZeroDel(argsZeroDel, repo, view);
        assertTrue(cmdZeroDel.execute() != 0, "DeleteCommand with zero ID (--id 0) must fail");

        // 4. Positional numeric query resolved by ID
        repo->addDirect({1, "Five", "cpp", "int x = 5;", {"algo"}, "", ""});
        auto argsPos = snipcli::controller::CliParser::parse({"get", "1"});
        snipcli::controller::commands::GetCommand cmdPos(argsPos, repo, view);
        assertTrue(cmdPos.execute() == 0, "GetCommand with positional numeric ID 1 must succeed");
    }

    std::cout << "[PASS] GoF Command Wiring tests passed. (25/25 Scenarios Verified)" << std::endl;
}
