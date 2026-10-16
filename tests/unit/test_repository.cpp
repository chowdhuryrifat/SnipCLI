#include "snipcli/model/json_snippet_repository.hpp"
#include <filesystem>
#include <iostream>

void runRepositoryTests() {
    std::cout << "[TEST] Running Repository tests..." << std::endl;
    std::filesystem::path testFile = std::filesystem::temp_directory_path() / "test_snipcli_repo.json";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }

    snipcli::model::JsonSnippetRepository repo(testFile);
    if (repo.count() != 0) {
        std::cerr << "Initial repo count is not 0!" << std::endl;
        std::exit(1);
    }

    snipcli::model::Snippet s1(0, "BFS", "cpp", "queue<int> q;", {"graph"}, "", "");
    uint64_t id1 = repo.save(s1);
    if (id1 != 1 || repo.count() != 1) {
        std::cerr << "Save snippet failed!" << std::endl;
        std::exit(1);
    }

    auto found = repo.getByTitle("BFS");
    if (!found.has_value() || found->getId() != 1 || found->getTitle() != "BFS") {
        std::cerr << "Get by title failed!" << std::endl;
        std::exit(1);
    }

    bool deleted = repo.removeByTitle("BFS");
    if (!deleted || repo.count() != 0) {
        std::cerr << "Remove by title failed!" << std::endl;
        std::exit(1);
    }

    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    std::cout << "[PASS] Repository tests passed." << std::endl;
}
