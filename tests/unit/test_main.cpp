#include <iostream>

void runSnippetTests();
void runValidatorTests();
void runRepositoryTests();
void runStrategyTests();
void runParserTests();
void runCommandTests();
void runViewTests();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Starting SnipCLI Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    runSnippetTests();
    runValidatorTests();
    runRepositoryTests();
    runStrategyTests();
    runParserTests();
    runCommandTests();
    runViewTests();

    std::cout << "========================================" << std::endl;
    std::cout << "All Unit Tests Passed Successfully! (7/7)" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
