#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/model/json_snippet_repository.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] Repository assertion failed: " << message << std::endl;
        std::exit(1);
    }
}

void setEnvVar(const char* name, const char* value) {
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void unsetEnvVar(const char* name) {
#if defined(_WIN32)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

}  // namespace

void runRepositoryTests() {
    std::cout << "[TEST] Running Repository tests..." << std::endl;

    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "snipcli_repo_tests";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    std::filesystem::path repoFile = testDir / "snippets.json";

    // ---------------------------------------------------------
    // Test 1: Empty Repository Invariants
    // ---------------------------------------------------------
    {
        snipcli::model::JsonSnippetRepository repo(repoFile);
        assertTrue(repo.count() == 0, "Initial repo count must be 0");
        assertTrue(repo.getAll().empty(), "Initial getAll() must be empty");
        assertTrue(!repo.getById(1).has_value(), "getById(1) must be nullopt on empty repo");
        assertTrue(!repo.getByTitle("BFS").has_value(), "getByTitle must be nullopt on empty repo");
        assertTrue(!repo.exists("BFS"), "exists() must be false on empty repo");
        assertTrue(!repo.removeById(1), "removeById must return false on empty repo");
        assertTrue(!repo.removeByTitle("BFS"), "removeByTitle must return false on empty repo");
    }

    // ---------------------------------------------------------
    // Test 2: ISP Interfaces, Insertion & Timestamps
    // ---------------------------------------------------------
    {
        snipcli::model::JsonSnippetRepository repo(repoFile);
        snipcli::model::ISnippetWriter& writer = repo;
        const snipcli::model::ISnippetReader& reader = repo;

        snipcli::model::Snippet s1(0, "BFS", "cpp", "queue<int> q;", {"graph", "traversal"}, "",
                                   "");
        uint64_t id1 = writer.save(s1);

        assertTrue(id1 == 1, "First snippet should receive ID 1");
        assertTrue(s1.getId() == 1, "Snippet entity mutated with assigned ID 1");
        assertTrue(!s1.getCreatedAt().empty(), "Snippet createdAt should be populated");
        assertTrue(!s1.getUpdatedAt().empty(), "Snippet updatedAt should be populated");
        assertTrue(reader.count() == 1, "Reader count must be 1");
        assertTrue(reader.exists("BFS"), "Snippet BFS must exist");

        auto found = reader.getByTitle("BFS");
        assertTrue(found.has_value(), "getByTitle('BFS') must return snippet");
        assertTrue(found->getId() == 1, "Found snippet ID must match");
        assertTrue(found->getLanguage() == "cpp", "Found snippet language must match");
        assertTrue(found->getCode() == "queue<int> q;", "Found snippet code must match");
        assertTrue(found->hasTag("graph") && found->hasTag("traversal"), "Found tags must match");
    }

    // ---------------------------------------------------------
    // Test 3: Auto-Increment ID Generation & getAll()
    // ---------------------------------------------------------
    {
        snipcli::model::JsonSnippetRepository repo(repoFile);
        snipcli::model::Snippet s2(0, "DFS", "cpp", "vector<bool> vis;", {"graph"}, "", "");
        snipcli::model::Snippet s3(0, "BinarySearch", "python", "l, r = 0, n - 1",
                                   {"binary-search"}, "", "");

        uint64_t id2 = repo.save(s2);
        uint64_t id3 = repo.save(s3);

        assertTrue(id2 == 2, "Second snippet should receive ID 2");
        assertTrue(id3 == 3, "Third snippet should receive ID 3");
        assertTrue(repo.count() == 3, "Total count must be 3");

        auto all = repo.getAll();
        assertTrue(all.size() == 3, "getAll() must return 3 snippets");
        assertTrue(all[0].getId() == 1 && all[1].getId() == 2 && all[2].getId() == 3,
                   "Snippets must have sequential IDs 1, 2, 3");
    }

    // ---------------------------------------------------------
    // Test 4: In-Place Update of Existing Snippet
    // ---------------------------------------------------------
    {
        snipcli::model::JsonSnippetRepository repo(repoFile);
        auto optS2 = repo.getById(2);
        assertTrue(optS2.has_value(), "Snippet 2 must exist before update");

        std::string originalCreatedAt = optS2->getCreatedAt();
        std::string originalUpdatedAt = optS2->getUpdatedAt();

        snipcli::model::Snippet updatedS2(2, "DFS_Recursive", "cpp", "void dfs(int u);",
                                          {"graph", "recursion"}, "", "");
        uint64_t returnedId = repo.save(updatedS2);

        assertTrue(returnedId == 2, "Updated snippet ID must remain 2");
        assertTrue(repo.count() == 3, "Count must remain 3 after in-place update");

        auto reloadedS2 = repo.getById(2);
        assertTrue(reloadedS2.has_value(), "Reloaded snippet 2 must exist");
        assertTrue(reloadedS2->getTitle() == "DFS_Recursive", "Title updated");
        assertTrue(reloadedS2->getCode() == "void dfs(int u);", "Code updated");
        assertTrue(reloadedS2->hasTag("recursion"), "New tag present");
        assertTrue(reloadedS2->getCreatedAt() == originalCreatedAt, "createdAt preserved");
    }

    // ---------------------------------------------------------
    // Test 5: Deletion by ID & Title
    // ---------------------------------------------------------
    {
        snipcli::model::JsonSnippetRepository repo(repoFile);

        // Delete non-existent ID
        assertTrue(!repo.removeById(9999), "removeById on invalid ID must return false");

        // Delete by ID
        assertTrue(repo.removeById(2), "removeById(2) must return true");
        assertTrue(repo.count() == 2, "Count should decrease to 2");
        assertTrue(!repo.getById(2).has_value(), "Snippet 2 should no longer exist");

        // Delete non-existent title
        assertTrue(!repo.removeByTitle("NonExistentTitle"),
                   "removeByTitle on missing title returns false");

        // Delete by Title
        assertTrue(repo.removeByTitle("BFS"), "removeByTitle('BFS') must return true");
        assertTrue(repo.count() == 1, "Count should decrease to 1");
        assertTrue(!repo.getByTitle("BFS").has_value(), "BFS should no longer exist");
        assertTrue(!repo.exists("BFS"), "exists('BFS') must be false");

        // Remaining snippet should be BinarySearch (ID 3)
        auto remaining = repo.getAll();
        assertTrue(remaining.size() == 1 && remaining[0].getTitle() == "BinarySearch",
                   "Only BinarySearch snippet should remain");
    }

    // ---------------------------------------------------------
    // Test 6: Round-Trip Special Characters & Escape Sequences
    // ---------------------------------------------------------
    {
        std::filesystem::path escapeFile = testDir / "escapes.json";
        snipcli::model::JsonSnippetRepository repo(escapeFile);

        std::string trickyCode =
            "#include <iostream>\n"
            "// Quotes: \"Hello\" and escaped quotes \\\"world\\\"\n"
            "// Backslashes: path\\\\to\\\\file\n"
            "// Tabs:\t\tIndent\n"
            "// Pseudo-JSON in code:\n"
            "const char* j = \"{\\\"tags\\\": [\\\"fake\\\"]}\";\n"
            "int main() { return 0; }\n";

        snipcli::model::Snippet trickySnippet(0, "Tricky_Escapes", "cpp", trickyCode,
                                              {"escape\"tag", "backslash\\tag"}, "", "");
        uint64_t id = repo.save(trickySnippet);
        assertTrue(id == 1, "Tricky snippet saved with ID 1");

        // Open via a completely fresh repository instance to verify disk persistence
        snipcli::model::JsonSnippetRepository freshRepo(escapeFile);
        auto loaded = freshRepo.getById(1);
        assertTrue(loaded.has_value(), "Loaded tricky snippet must exist");
        assertTrue(loaded->getTitle() == "Tricky_Escapes", "Title matches");
        assertTrue(loaded->getCode() == trickyCode, "Code with complex escapes must match exactly");
        assertTrue(loaded->hasTag("escape\"tag"), "Tag with quote matches");
        assertTrue(loaded->hasTag("backslash\\tag"), "Tag with backslash matches");
    }

    // ---------------------------------------------------------
    // Test 7: Nested Storage Directory Creation
    // ---------------------------------------------------------
    {
        std::filesystem::path nestedPath = testDir / "a" / "b" / "c" / "nested_repo.json";
        snipcli::model::JsonSnippetRepository nestedRepo(nestedPath);
        snipcli::model::Snippet s(0, "Nested", "cpp", "int a = 1;", {}, "", "");
        uint64_t id = nestedRepo.save(s);
        assertTrue(id == 1, "Saved into deeply nested directory");
        assertTrue(std::filesystem::exists(nestedPath), "Nested file was created on disk");
        assertTrue(nestedRepo.getStoragePath() == nestedPath, "Storage path getter matches");
    }

    // ---------------------------------------------------------
    // Test 8: Default Storage Path & SNIPCLI_STORAGE_PATH Override
    // ---------------------------------------------------------
    {
        // Default path resolution
        std::filesystem::path defaultPath =
            snipcli::model::JsonSnippetRepository::getDefaultStoragePath();
        assertTrue(!defaultPath.empty(), "getDefaultStoragePath() must not be empty");

        // Environment override
        std::filesystem::path customOverride = testDir / "custom_override.json";
        setEnvVar("SNIPCLI_STORAGE_PATH", customOverride.string().c_str());

        std::filesystem::path resolved =
            snipcli::model::JsonSnippetRepository::getDefaultStoragePath();
        assertTrue(resolved == customOverride, "SNIPCLI_STORAGE_PATH must override default path");

        // Default constructor uses resolved path
        snipcli::model::JsonSnippetRepository defaultRepo;
        assertTrue(defaultRepo.getStoragePath() == customOverride,
                   "Default constructor must use resolved path from env override");

        unsetEnvVar("SNIPCLI_STORAGE_PATH");
    }

    // ---------------------------------------------------------
    // Test 9: Thread Safety & Concurrent Writes
    // ---------------------------------------------------------
    {
        std::filesystem::path concurrentFile = testDir / "concurrent_writes.json";
        snipcli::model::JsonSnippetRepository concurrentRepo(concurrentFile);

        const int numThreads = 8;
        const int snippetsPerThread = 5;
        std::vector<std::thread> workers;
        workers.reserve(static_cast<size_t>(numThreads));

        for (int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&concurrentRepo, t, snippetsPerThread]() {
                for (int i = 0; i < snippetsPerThread; ++i) {
                    std::string title =
                        "Thread_" + std::to_string(t) + "_Snip_" + std::to_string(i);
                    std::string code = "int val = " + std::to_string(t * 100 + i) + ";";
                    snipcli::model::Snippet s(0, title, "cpp", code, {"thread", "concurrent"}, "",
                                              "");
                    (void)concurrentRepo.save(s);
                }
            });
        }

        for (auto& w : workers) {
            w.join();
        }

        const size_t expectedTotal = static_cast<size_t>(numThreads * snippetsPerThread);
        assertTrue(concurrentRepo.count() == expectedTotal,
                   "Concurrent writes: count must equal " + std::to_string(expectedTotal));

        auto allSnippets = concurrentRepo.getAll();
        assertTrue(allSnippets.size() == expectedTotal,
                   "Concurrent writes: getAll size must equal " + std::to_string(expectedTotal));

        // Verify all IDs are unique between 1 and expectedTotal
        std::vector<uint64_t> ids;
        ids.reserve(allSnippets.size());
        for (const auto& s : allSnippets) {
            ids.push_back(s.getId());
        }
        std::sort(ids.begin(), ids.end());
        for (size_t i = 0; i < ids.size(); ++i) {
            assertTrue(ids[i] == static_cast<uint64_t>(i + 1),
                       "Concurrent write IDs must be contiguous sequence from 1 to " +
                           std::to_string(expectedTotal));
        }
    }

    // ---------------------------------------------------------
    // Test 10: Thread Safety with Concurrent Reads and Writes
    // ---------------------------------------------------------
    {
        std::filesystem::path mixedFile = testDir / "concurrent_mixed.json";
        snipcli::model::JsonSnippetRepository mixedRepo(mixedFile);

        // Prepopulate 10 snippets
        for (int i = 1; i <= 10; ++i) {
            snipcli::model::Snippet s(0, "Initial_" + std::to_string(i), "cpp", "code();", {"init"},
                                      "", "");
            (void)mixedRepo.save(s);
        }

        std::vector<std::thread> threads;
        // Reader threads
        for (int r = 0; r < 4; ++r) {
            threads.emplace_back([&mixedRepo]() {
                for (int i = 0; i < 20; ++i) {
                    auto count = mixedRepo.count();
                    assertTrue(count >= 10, "Count must be at least initial 10");
                    auto opt = mixedRepo.getById(1);
                    assertTrue(opt.has_value(), "Snippet 1 must exist");
                    auto all = mixedRepo.getAll();
                    assertTrue(!all.empty(), "getAll must not be empty");
                }
            });
        }

        // Writer threads
        for (int w = 0; w < 4; ++w) {
            threads.emplace_back([&mixedRepo, w]() {
                for (int i = 0; i < 5; ++i) {
                    std::string title =
                        "MixedWriter_" + std::to_string(w) + "_" + std::to_string(i);
                    snipcli::model::Snippet s(0, title, "python", "pass", {"mixed"}, "", "");
                    (void)mixedRepo.save(s);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        assertTrue(mixedRepo.count() == 10 + (4 * 5),
                   "Mixed concurrent operations preserved all records");
    }

    // Cleanup test directory
    std::filesystem::remove_all(testDir, ec);

    std::cout << "[PASS] Repository tests passed. (10/10 Invariants & Scenarios Verified)"
              << std::endl;
}
