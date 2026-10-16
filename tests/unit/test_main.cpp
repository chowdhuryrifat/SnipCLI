#include <iostream>

void runSnippetTests();
void runValidatorTests();
void runRepositoryTests();
void runStrategyTests();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Starting SnipCLI Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    runSnippetTests();
    runValidatorTests();
    runRepositoryTests();
    runStrategyTests();

    std::cout << "========================================" << std::endl;
    std::cout << "All Unit Tests Passed Successfully! (4/4)" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
