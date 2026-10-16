#include <filesystem>
#include <iostream>

#include "snipcli/model/json_snippet_repository.hpp"

void runRepositoryTests() {
    std::cout << "[TEST] Running Repository tests..." << std::endl;
    std::filesystem::path testFile =
        std::filesystem::temp_directory_path() / "test_snipcli_repo.json";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }

    snipcli::model::JsonSnippetRepository repo(testFile);
    if (repo.count() != 0) {
        std::cerr << "Initial repo count is not 0!" << std::endl;
        std::exit(1);
    }

    // Test ISP: Verify operations via segregated ISnippetWriter interface
    snipcli::model::ISnippetWriter& writer = repo;
    snipcli::model::Snippet s1(0, "BFS", "cpp", "queue<int> q;", {"graph"}, "", "");
    uint64_t id1 = writer.save(s1);
    if (id1 != 1) {
        std::cerr << "Save snippet via ISnippetWriter failed!" << std::endl;
        std::exit(1);
    }

    // Test ISP: Verify queries via segregated ISnippetReader interface
    const snipcli::model::ISnippetReader& reader = repo;
    if (reader.count() != 1 || !reader.exists("BFS")) {
        std::cerr << "Query via ISnippetReader failed!" << std::endl;
        std::exit(1);
    }

    auto found = reader.getByTitle("BFS");
    if (!found.has_value() || found->getId() != 1 || found->getTitle() != "BFS") {
        std::cerr << "Get by title via ISnippetReader failed!" << std::endl;
        std::exit(1);
    }

    auto byId = reader.getById(1);
    if (!byId.has_value() || byId->getTitle() != "BFS") {
        std::cerr << "Get by ID via ISnippetReader failed!" << std::endl;
        std::exit(1);
    }

    bool deleted = writer.removeByTitle("BFS");
    if (!deleted || reader.count() != 0) {
        std::cerr << "Remove by title via ISnippetWriter failed!" << std::endl;
        std::exit(1);
    }

    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    std::cout << "[PASS] Repository tests passed." << std::endl;
}
