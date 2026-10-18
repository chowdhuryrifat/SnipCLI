#include <iostream>

void runSnippetTests();
void runValidatorTests();
void runRepositoryTests();
void runStrategyTests();
void runParserTests();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Starting SnipCLI Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    runSnippetTests();
    runValidatorTests();
    runRepositoryTests();
    runStrategyTests();
    runParserTests();

    std::cout << "========================================" << std::endl;
    std::cout << "All Unit Tests Passed Successfully! (5/5)" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
