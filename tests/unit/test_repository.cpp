#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "scoped_env.hpp"
#include "snipcli/common/exceptions.hpp"
#include "snipcli/model/json_snippet_repository.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] Repository assertion failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runRepositoryTests() {
    std::cout << "[TEST] Running Repository tests..." << std::endl;

    // Isolate repository unit tests from external lock timeout overrides (e.g.
    // SNIPCLI_LOCK_TIMEOUT_MS=1)
    snipcli::test::ScopedEnv lockTimeoutGuard("SNIPCLI_LOCK_TIMEOUT_MS", "10000");

    static_assert(std::is_nothrow_move_assignable_v<snipcli::model::Snippet>,
                  "Snippet must be nothrow move assignable for strong exception safety");
    static_assert(
        std::is_base_of_v<snipcli::common::SnipException, snipcli::common::StorageException>,
        "StorageException must derive from SnipException");
    static_assert(
        std::is_base_of_v<snipcli::common::SnipException, snipcli::common::ValidationException>,
        "ValidationException must derive from SnipException");

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
        // First isolate and clear any external SNIPCLI_STORAGE_PATH
        snipcli::test::ScopedEnv clearEnv("SNIPCLI_STORAGE_PATH", std::nullopt);
        // Ensure HOME or APPDATA exists so getDefaultStoragePath() has an OS anchor even under env
        // -i
        std::optional<snipcli::test::ScopedEnv> dummyHome;
        if (!std::getenv("HOME") && !std::getenv("APPDATA")) {
            dummyHome.emplace("HOME", testDir.string());
        }

        // Default path resolution
        std::filesystem::path defaultPath =
            snipcli::model::JsonSnippetRepository::getDefaultStoragePath();
        assertTrue(!defaultPath.empty(), "getDefaultStoragePath() must not be empty");

        // Environment override
        std::filesystem::path customOverride = testDir / "custom_override.json";
        {
            snipcli::test::ScopedEnv overrideEnv("SNIPCLI_STORAGE_PATH", customOverride.string());

            std::filesystem::path resolved =
                snipcli::model::JsonSnippetRepository::getDefaultStoragePath();
            assertTrue(resolved == customOverride,
                       "SNIPCLI_STORAGE_PATH must override default path");

            // Default constructor uses resolved path
            snipcli::model::JsonSnippetRepository defaultRepo;
            assertTrue(defaultRepo.getStoragePath() == customOverride,
                       "Default constructor must use resolved path from env override");
        }
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

        std::atomic<bool> workerSuccess{true};
        std::vector<std::thread> threads;
        // Reader threads (recording failures safely without calling std::exit from child threads)
        for (int r = 0; r < 4; ++r) {
            threads.emplace_back([&mixedRepo, &workerSuccess]() {
                for (int i = 0; i < 20; ++i) {
                    auto count = mixedRepo.count();
                    if (count < 10) {
                        workerSuccess.store(false);
                    }
                    auto opt = mixedRepo.getById(1);
                    if (!opt.has_value()) {
                        workerSuccess.store(false);
                    }
                    auto all = mixedRepo.getAll();
                    if (all.empty()) {
                        workerSuccess.store(false);
                    }
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

        assertTrue(workerSuccess.load(), "Concurrent reader threads completed without errors");
        assertTrue(mixedRepo.count() == 10 + (4 * 5),
                   "Mixed concurrent operations preserved all records");
    }

    // ---------------------------------------------------------
    // Test 11: Fail-Closed Corrupted JSON Handling & Quarantine
    // ---------------------------------------------------------
    {
        std::filesystem::path corruptFile = testDir / "corrupted_store.json";

        // 11a: Empty and whitespace-only file returns count 0
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "   \n\t  \r\n";
        }
        snipcli::model::JsonSnippetRepository emptyRepo(corruptFile);
        assertTrue(emptyRepo.count() == 0, "Whitespace-only JSON file treated as empty repository");
        assertTrue(emptyRepo.getAll().empty(), "Whitespace-only JSON getAll is empty");

        // 11b: Truncated JSON file throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "[\n  {\n    \"id\": 1,\n    \"title\": \"Truncated\",\n    \"code\": \"int x;";
        }
        bool threwTruncated = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwTruncated = true;
        }
        assertTrue(threwTruncated, "Truncated JSON file correctly triggers StorageException");

        // 11c: Non-array root token throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "{\"not_an_array\": true}";
        }
        bool threwNonArray = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwNonArray = true;
        }
        assertTrue(threwNonArray, "Non-array root JSON object correctly triggers StorageException");

        // 11d: Missing required field (missing code) throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "[\n  {\n    \"id\": 1,\n    \"title\": \"NoCode\",\n    \"language\": "
                   "\"cpp\"\n  }\n]";
        }
        bool threwMissingCode = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwMissingCode = true;
        }
        assertTrue(threwMissingCode,
                   "Snippet missing mandatory code field triggers StorageException");

        // 11e: Invalid escape sequence (\q) throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "[\n  {\n    \"id\": 1,\n    \"title\": \"BadEscape\",\n    \"language\": "
                   "\"cpp\",\n    \"code\": \"int \\q = 0;\"\n  }\n]";
        }
        bool threwBadEscape = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwBadEscape = true;
        }
        assertTrue(threwBadEscape, "Invalid escape sequence \\q triggers StorageException");

        // 11f: Non-hexadecimal \u escape (\u12zz) throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "[\n  {\n    \"id\": 1,\n    \"title\": \"BadHex\",\n    \"language\": "
                   "\"cpp\",\n    \"code\": \"int x = \\u12zz;\"\n  }\n]";
        }
        bool threwBadHex = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwBadHex = true;
        }
        assertTrue(threwBadHex, "Non-hex sequence in \\u escape triggers StorageException");

        // 11g: Isolated high surrogate (\ud800 without low surrogate) throws StorageException
        {
            std::ofstream out(corruptFile, std::ios::trunc);
            out << "[\n  {\n    \"id\": 1,\n    \"title\": \"IsolatedSurrogate\",\n    "
                   "\"language\": \"cpp\",\n    \"code\": \"// \\ud800 lonely\"\n  }\n]";
        }
        bool threwSurrogate = false;
        try {
            (void)emptyRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            threwSurrogate = true;
        }
        assertTrue(threwSurrogate, "Isolated high surrogate triggers StorageException");
    }

    // ---------------------------------------------------------
    // Test 12: Unicode \\uXXXX Decoding & UTF-8 Round-Trip
    // ---------------------------------------------------------
    {
        std::filesystem::path unicodeFile = testDir / "unicode_store.json";
        {
            std::ofstream out(unicodeFile, std::ios::trunc);
            out << "[\n"
                << "  {\n"
                << "    \"id\": 1,\n"
                << "    \"title\": \"UnicodeTest\",\n"
                << "    \"language\": \"cpp\",\n"
                << "    \"code\": \"// caf\\u00e9 \\u4e2d\\u6587 \\ud83d\\ude00\",\n"
                << "    \"tags\": [\"unicode\"]\n"
                << "  }\n"
                << "]\n";
        }

        snipcli::model::JsonSnippetRepository unicodeRepo(unicodeFile);
        auto opt = unicodeRepo.getByTitle("UnicodeTest");
        assertTrue(opt.has_value(), "Unicode snippet found by title");
        assertTrue(opt->getCode() == "// café 中文 😀",
                   "Unicode \\u escapes correctly decoded to UTF-8 including surrogate pairs");

        // Verify save preserves the UTF-8 bytes intact
        snipcli::model::Snippet copy = *opt;
        (void)unicodeRepo.save(copy);
        auto reloaded = unicodeRepo.getByTitle("UnicodeTest");
        assertTrue(reloaded.has_value(), "Reloaded unicode snippet found");
        assertTrue(reloaded->getCode() == "// café 中文 😀",
                   "UTF-8 code roundtrips cleanly across save and reload");
    }

    // ---------------------------------------------------------
    // Test 13: Atomic Title Uniqueness & Strong Exception Safety
    // ---------------------------------------------------------
    {
        std::filesystem::path uniqFile = testDir / "uniqueness_store.json";
        snipcli::model::JsonSnippetRepository uniqRepo(uniqFile);

        snipcli::model::Snippet s1(0, "UniqueTitle", "cpp", "code();", {"algo"}, "", "");
        uint64_t id1 = uniqRepo.save(s1);
        assertTrue(id1 == 1, "First snippet saved with ID 1");

        // Attempt duplicate insertion
        snipcli::model::Snippet dup(0, "UniqueTitle", "python", "pass", {"algo"}, "", "");
        bool duplicateCaught = false;
        try {
            (void)uniqRepo.save(dup);
        } catch (const snipcli::common::ValidationException&) {
            duplicateCaught = true;
        }
        assertTrue(duplicateCaught, "Duplicate snippet title correctly rejected by repository");
        assertTrue(dup.getId() == 0,
                   "Caller's snippet ID remains 0 upon rejected save (Strong Exception Safety)");
        assertTrue(dup.getCreatedAt().empty(),
                   "Caller's snippet createdAt remains empty upon rejected save");

        // Updating snippet 1 with its own title is allowed
        s1.setCode("new_code();");
        uint64_t updateId = uniqRepo.save(s1);
        assertTrue(updateId == 1, "In-place update with existing title succeeded");

        // Creating snippet 2 and attempting to rename to snippet 1's title fails
        snipcli::model::Snippet s2(0, "SecondTitle", "cpp", "s2();", {"test"}, "", "");
        uint64_t id2 = uniqRepo.save(s2);
        assertTrue(id2 == 2, "Second snippet saved with ID 2");

        s2.setTitle("UniqueTitle");  // Attempt collision with s1
        bool renameConflictCaught = false;
        try {
            (void)uniqRepo.save(s2);
        } catch (const snipcli::common::ValidationException&) {
            renameConflictCaught = true;
        }
        assertTrue(renameConflictCaught,
                   "Renaming snippet to existing title rejected with ValidationException");
    }

    // ---------------------------------------------------------
    // Test 14: Directory Storage Path Rejection & Deferred I/O
    // ---------------------------------------------------------
    {
        std::filesystem::path dirStore = testDir / "sub_dir_store";
        std::filesystem::create_directories(dirStore, ec);

        snipcli::model::JsonSnippetRepository dirRepo(dirStore);
        bool dirExCaught = false;
        try {
            (void)dirRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            dirExCaught = true;
        }
        assertTrue(dirExCaught, "getAll() on directory path must throw StorageException");

        // Verify deferred directory creation: constructing repo must not create parent directory
        std::filesystem::path deferredDir = testDir / "deferred_dir";
        std::filesystem::path deferredFile = deferredDir / "nested" / "store.json";
        assertTrue(!std::filesystem::exists(deferredDir),
                   "Directory must not exist before repo constructor");
        snipcli::model::JsonSnippetRepository deferredRepo(deferredFile);
        assertTrue(!std::filesystem::exists(deferredDir),
                   "Repo constructor must not create directory (I/O free)");

        snipcli::model::Snippet snip(0, "DeferredTest", "cpp", "int x;", {}, "", "");
        uint64_t id = deferredRepo.save(snip);
        assertTrue(id == 1, "Save succeeded on deferred repo");
        assertTrue(std::filesystem::exists(deferredFile),
                   "Storage file and parent directories created upon save");
    }

    // ---------------------------------------------------------
    // Test 15: UTF-8 BOM Stripping
    // ---------------------------------------------------------
    {
        std::filesystem::path bomFile = testDir / "bom_store.json";
        {
            std::ofstream out(bomFile, std::ios::binary | std::ios::trunc);
            // Write UTF-8 BOM (0xEF, 0xBB, 0xBF) followed by JSON array
            out << "\xEF\xBB\xBF[\n"
                << "  {\n"
                << "    \"id\": 1,\n"
                << "    \"title\": \"BomTest\",\n"
                << "    \"language\": \"cpp\",\n"
                << "    \"code\": \"int bom = 1;\",\n"
                << "    \"tags\": [\"bom\"]\n"
                << "  }\n"
                << "]\n";
        }
        snipcli::model::JsonSnippetRepository bomRepo(bomFile);
        auto snippets = bomRepo.getAll();
        assertTrue(snippets.size() == 1, "BOM file correctly loaded 1 snippet");
        assertTrue(snippets[0].getTitle() == "BomTest", "BOM snippet title matches");
    }

    // ---------------------------------------------------------
    // Test 16: Strict ID & Schema Validation in Storage
    // ---------------------------------------------------------
    {
        // 16a: Missing ID
        std::filesystem::path noIdFile = testDir / "no_id.json";
        {
            std::ofstream out(noIdFile, std::ios::trunc);
            out << "[{\"title\":\"NoId\",\"language\":\"cpp\",\"code\":\"x\",\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository noIdRepo(noIdFile);
        bool noIdCaught = false;
        try {
            (void)noIdRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            noIdCaught = true;
        }
        assertTrue(noIdCaught, "Missing ID field must throw StorageException");

        // 16b: Zero ID
        std::filesystem::path zeroIdFile = testDir / "zero_id.json";
        {
            std::ofstream out(zeroIdFile, std::ios::trunc);
            out << "[{\"id\":0,\"title\":\"ZeroId\",\"language\":\"cpp\",\"code\":\"x\",\"tags\":[]"
                   "}]";
        }
        snipcli::model::JsonSnippetRepository zeroIdRepo(zeroIdFile);
        bool zeroIdCaught = false;
        try {
            (void)zeroIdRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            zeroIdCaught = true;
        }
        assertTrue(zeroIdCaught, "ID 0 must throw StorageException");

        // 16c: Duplicate IDs
        std::filesystem::path dupIdFile = testDir / "dup_id.json";
        {
            std::ofstream out(dupIdFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"D1\",\"language\":\"cpp\",\"code\":\"x\",\"tags\":[]},"
                << "{\"id\":1,\"title\":\"D2\",\"language\":\"cpp\",\"code\":\"y\",\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository dupIdRepo(dupIdFile);
        bool dupIdCaught = false;
        try {
            (void)dupIdRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            dupIdCaught = true;
        }
        assertTrue(dupIdCaught, "Duplicate IDs must throw StorageException");

        // 16d: Max uint64 ID overflow prevention on next save
        std::filesystem::path maxIdFile = testDir / "max_id.json";
        {
            std::ofstream out(maxIdFile, std::ios::trunc);
            out << "[{\"id\":18446744073709551615,\"title\":\"MaxId\",\"language\":\"cpp\","
                   "\"code\":\"x\",\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository maxIdRepo(maxIdFile);
        snipcli::model::Snippet next(0, "Next", "cpp", "z", {}, "", "");
        bool overflowCaught = false;
        try {
            (void)maxIdRepo.save(next);
        } catch (const snipcli::common::StorageException&) {
            overflowCaught = true;
        }
        assertTrue(overflowCaught, "Saving after max uint64 ID must throw StorageException");
    }

    // ---------------------------------------------------------
    // Test 17: Parsing Strictness (NUL, Control Chars, Garbage, Unterminated)
    // ---------------------------------------------------------
    {
        // 17a: Embedded NUL \u0000
        std::filesystem::path nulFile = testDir / "nul.json";
        {
            std::ofstream out(nulFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"Nul\",\"language\":\"cpp\",\"code\":\"a\\u0000b\","
                   "\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository nulRepo(nulFile);
        bool nulCaught = false;
        try {
            (void)nulRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            nulCaught = true;
        }
        assertTrue(nulCaught, "Embedded NUL \\u0000 must throw StorageException");

        // 17b: Inter-object garbage
        std::filesystem::path garbageFile = testDir / "garbage.json";
        {
            std::ofstream out(garbageFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"A\",\"language\":\"cpp\",\"code\":\"a\",\"tags\":[]} "
                   "GARBAGE "
                   "{\"id\":2,\"title\":\"B\",\"language\":\"cpp\",\"code\":\"b\",\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository garbageRepo(garbageFile);
        bool garbageCaught = false;
        try {
            (void)garbageRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            garbageCaught = true;
        }
        assertTrue(garbageCaught, "Inter-object garbage must throw StorageException");

        // 17c: Unescaped literal newline inside string
        std::filesystem::path newlineFile = testDir / "raw_newline.json";
        {
            std::ofstream out(newlineFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"A\",\"language\":\"cpp\",\"code\":\"line1\nline2\","
                   "\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository newlineRepo(newlineFile);
        bool newlineCaught = false;
        try {
            (void)newlineRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            newlineCaught = true;
        }
        assertTrue(newlineCaught,
                   "Unescaped literal newline inside JSON string must throw StorageException");

        // 17d: Unterminated JSON array (missing closing bracket)
        std::filesystem::path untermFile = testDir / "unterm.json";
        {
            std::ofstream out(untermFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"A\",\"language\":\"cpp\",\"code\":\"a\",\"tags\":[]}";
        }
        snipcli::model::JsonSnippetRepository untermRepo(untermFile);
        bool untermCaught = false;
        try {
            (void)untermRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            untermCaught = true;
        }
        assertTrue(untermCaught, "Unterminated JSON array must throw StorageException");
    }

    // ---------------------------------------------------------
    // Test 18: Symlink Preservation & File Permissions
    // ---------------------------------------------------------
    {
        std::filesystem::path realDir = testDir / "real_dir";
        std::filesystem::create_directories(realDir, ec);
        std::filesystem::path realFile = realDir / "target.json";
        std::filesystem::path symlinkFile = testDir / "symlink_store.json";

        snipcli::model::JsonSnippetRepository initRepo(realFile);
        snipcli::model::Snippet s1(0, "Sym1", "cpp", "int a;", {}, "", "");
        (void)initRepo.save(s1);

        // Create symlink
        std::filesystem::create_symlink(realFile, symlinkFile, ec);
        if (!ec) {
            snipcli::model::JsonSnippetRepository symRepo(symlinkFile);
            snipcli::model::Snippet s2(0, "Sym2", "cpp", "int b;", {}, "", "");
            (void)symRepo.save(s2);

            assertTrue(std::filesystem::is_symlink(symlinkFile),
                       "Symlink must be preserved after save()");
            snipcli::model::JsonSnippetRepository verifyRepo(realFile);
            assertTrue(verifyRepo.count() == 2,
                       "Target file must contain both snippets after saving via symlink");
        }
    }

    // ---------------------------------------------------------
    // Test 19: Case-Insensitive Title Collision & Lookup
    // ---------------------------------------------------------
    {
        std::filesystem::path caseFile = testDir / "case_store.json";
        snipcli::model::JsonSnippetRepository caseRepo(caseFile);

        snipcli::model::Snippet s1(0, "bfs", "cpp", "int a;", {}, "", "");
        (void)caseRepo.save(s1);

        snipcli::model::Snippet s2(0, "BFS", "cpp", "int b;", {}, "", "");
        bool caseCollisionCaught = false;
        try {
            (void)caseRepo.save(s2);
        } catch (const snipcli::common::ValidationException&) {
            caseCollisionCaught = true;
        }
        assertTrue(caseCollisionCaught,
                   "Case-insensitive title collision must throw ValidationException");

        // Lookup with different casing succeeds
        auto found = caseRepo.getByTitle("BFS");
        assertTrue(found.has_value(), "getByTitle matches case-insensitively");
        assertTrue(found->getTitle() == "bfs", "Original casing preserved");

        // Remove by title matches case-insensitively
        assertTrue(caseRepo.removeByTitle("BfS"), "removeByTitle matches case-insensitively");
        assertTrue(caseRepo.count() == 0, "Snippet removed successfully");
    }

    // ---------------------------------------------------------
    // Test 20: Multi-Instance FileLock Concurrency & Hardening
    // ---------------------------------------------------------
    {
        // 20a: Two repository instances on the same file with 8 concurrent threads (R2a)
        std::filesystem::path concurrentFile = testDir / "concurrent_store.json";
        snipcli::model::JsonSnippetRepository repoA(concurrentFile), repoB(concurrentFile);
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&, t] {
                auto& repo = (t % 2) ? repoA : repoB;
                for (int i = 0; i < 5; ++i) {
                    snipcli::model::Snippet s(0, "T" + std::to_string(t) + "_" + std::to_string(i),
                                              "cpp", "int x;", {}, "", "");
                    (void)repo.save(s);
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
        assertTrue(repoA.count() == 40, "Two repository instances must not lose updates");
        auto allSnippets = repoA.getAll();
        std::unordered_set<uint64_t> distinctIds;
        for (const auto& s : allSnippets) {
            distinctIds.insert(s.getId());
        }
        assertTrue(distinctIds.size() == 40,
                   "All 40 IDs across separate instances must be distinct");

        // 20b: Nested JSON object in storage must not hoist title or drop code (R2a)
        std::filesystem::path nestedFile = testDir / "nested_store.json";
        {
            std::ofstream out(nestedFile, std::ios::trunc);
            out << "[\n"
                << "  {\n"
                << "    \"id\": 1,\n"
                << "    \"meta\": {\"title\": \"nested_dummy\", \"author\": \"alice\"},\n"
                << "    \"title\": \"RealTitle\",\n"
                << "    \"language\": \"cpp\",\n"
                << "    \"code\": \"int realCode = 42;\",\n"
                << "    \"tags\": [\"tree\"]\n"
                << "  }\n"
                << "]\n";
        }
        snipcli::model::JsonSnippetRepository nestedRepo(nestedFile);
        auto nestedOpt = nestedRepo.getByTitle("RealTitle");
        assertTrue(nestedOpt.has_value(), "Snippet with nested object retrieved by actual title");
        assertTrue(nestedOpt->getTitle() == "RealTitle",
                   "Actual title not overwritten by nested object");
        assertTrue(nestedOpt->getCode() == "int realCode = 42;",
                   "Code preserved with nested object");

        // 20c: Duplicate titles in hand-edited file rejected on load (R4)
        std::filesystem::path dupTitleFile = testDir / "duptitle_store.json";
        {
            std::ofstream out(dupTitleFile, std::ios::trunc);
            out << "[{\"id\":1,\"title\":\"bfs\",\"language\":\"cpp\",\"code\":\"a\",\"tags\":[]},"
                << "{\"id\":2,\"title\":\"BFS\",\"language\":\"cpp\",\"code\":\"b\",\"tags\":[]}]";
        }
        snipcli::model::JsonSnippetRepository dupTitleRepo(dupTitleFile);
        bool dupTitleCaught = false;
        try {
            (void)dupTitleRepo.getAll();
        } catch (const snipcli::common::StorageException&) {
            dupTitleCaught = true;
        }
        assertTrue(dupTitleCaught,
                   "Duplicate titles (case-insensitive) in hand-edited file must throw "
                   "StorageException");

        // 20d: Storage path alias locking with symlinked parent directory (Review finding on PR
        // #21)
        std::filesystem::path realParent = testDir / "real_alias_store";
        std::filesystem::path linkParent = testDir / "link_alias_store";
        std::filesystem::create_directories(realParent, ec);
        std::filesystem::create_directory_symlink(realParent, linkParent, ec);
        if (!ec) {
            std::filesystem::path fileViaReal = realParent / "alias_store.json";
            std::filesystem::path fileViaLink = linkParent / "alias_store.json";

            snipcli::model::JsonSnippetRepository repoViaReal(fileViaReal);
            snipcli::model::JsonSnippetRepository repoViaLink(fileViaLink);

            std::vector<std::thread> aliasWorkers;
            for (int t = 0; t < 8; ++t) {
                aliasWorkers.emplace_back([&, t] {
                    auto& repo = (t % 2) ? repoViaReal : repoViaLink;
                    for (int i = 0; i < 5; ++i) {
                        snipcli::model::Snippet s(
                            0, "Alias_" + std::to_string(t) + "_" + std::to_string(i), "cpp",
                            "int a = 1;", {}, "", "");
                        (void)repo.save(s);
                    }
                });
            }
            for (auto& w : aliasWorkers) {
                w.join();
            }

            assertTrue(repoViaReal.count() == 40,
                       "All 40 writes across real and symlinked parent aliases must persist");
            assertTrue(repoViaLink.count() == 40,
                       "Repository via symlinked path sees all 40 writes");
            auto aliasSnippets = repoViaReal.getAll();
            std::unordered_set<uint64_t> aliasDistinctIds;
            for (const auto& s : aliasSnippets) {
                aliasDistinctIds.insert(s.getId());
            }
            assertTrue(aliasDistinctIds.size() == 40,
                       "All 40 IDs across symlink alias instances must be distinct");
        }
    }

    // ---------------------------------------------------------
    // 21: Symlink loop and recursion depth defense (F1)
    // ---------------------------------------------------------
    {
        std::filesystem::path symloopDir = testDir / "symloop_test";
        std::filesystem::create_directories(symloopDir, ec);

        // (a) 2-link loop: a -> b, b -> a
        std::filesystem::path linkA = symloopDir / "link_a.json";
        std::filesystem::path linkB = symloopDir / "link_b.json";
        std::filesystem::create_symlink(linkB, linkA, ec);
        if (!ec) {
            std::filesystem::create_symlink(linkA, linkB, ec);
            if (!ec) {
                bool caught2Hop = false;
                try {
                    snipcli::model::JsonSnippetRepository loopRepo(linkA);
                    (void)loopRepo.getAll();
                } catch (const snipcli::common::StorageException& ex) {
                    caught2Hop = true;
                    assertTrue(std::string(ex.what()).find("Too many levels of symbolic links") !=
                                   std::string::npos,
                               "StorageException message mentions symlink loop");
                }
                assertTrue(caught2Hop, "2-link symlink loop throws StorageException");
            }
        }

        // (b) Self-link: loop_self -> loop_self
        ec.clear();
        std::filesystem::path selfLink = symloopDir / "self_link.json";
        std::filesystem::create_symlink(selfLink, selfLink, ec);
        if (!ec) {
            bool caughtSelf = false;
            try {
                snipcli::model::JsonSnippetRepository selfRepo(selfLink);
                (void)selfRepo.getAll();
            } catch (const snipcli::common::StorageException& ex) {
                caughtSelf = true;
                assertTrue(std::string(ex.what()).find("Too many levels of symbolic links") !=
                               std::string::npos,
                           "Self-link throws StorageException with symlink loop message");
            }
            assertTrue(caughtSelf, "Self-symlink loop throws StorageException");
        }

        // (c) 41-hop symlink chain (exceeds kMaxSymlinkHops = 40)
        ec.clear();
        std::filesystem::path targetFile = symloopDir / "real_target.json";
        {
            std::ofstream out(targetFile);
            out << "[]";
        }
        std::filesystem::path prev = targetFile;
        bool chainCreated = true;
        for (int i = 41; i >= 1; --i) {
            std::filesystem::path currentLink = symloopDir / ("hop_" + std::to_string(i) + ".json");
            std::filesystem::create_symlink(prev, currentLink, ec);
            if (ec) {
                chainCreated = false;
                break;
            }
            prev = currentLink;
        }
        if (chainCreated) {
            bool caught41Hop = false;
            try {
                snipcli::model::JsonSnippetRepository hopRepo(prev);
                (void)hopRepo.getAll();
            } catch (const snipcli::common::StorageException& ex) {
                caught41Hop = true;
                assertTrue(std::string(ex.what()).find("Too many levels of symbolic links") !=
                               std::string::npos,
                           "41-hop symlink chain throws StorageException with depth message");
            }
            assertTrue(caught41Hop, "41-hop symlink chain throws StorageException");
        }
    }

    // Cleanup test directory
    std::filesystem::remove_all(testDir, ec);

    std::cout << "[PASS] Repository tests passed. (21/21 Invariants & Scenarios Verified)"
              << std::endl;
}
