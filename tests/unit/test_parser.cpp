#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/command_dispatcher.hpp"
#include "snipcli/controller/i_command.hpp"

namespace {

class MockCommand : public snipcli::controller::ICommand {
public:
    MockCommand(std::string name, std::string desc, int returnCode = 0)
        : m_name(std::move(name)), m_desc(std::move(desc)), m_returnCode(returnCode) {}

    int execute() override {
        m_executed = true;
        return m_returnCode;
    }

    [[nodiscard]] std::string getName() const override { return m_name; }

    [[nodiscard]] std::string getDescription() const override { return m_desc; }

    [[nodiscard]] bool wasExecuted() const noexcept { return m_executed; }

private:
    std::string m_name;
    std::string m_desc;
    int m_returnCode;
    bool m_executed{false};
};

}  // namespace

void runParserTests() {
    std::cout << "[TEST] Running CliParser & CommandDispatcher tests..." << std::endl;

    // Test 1: Subcommand verb and space-separated options
    {
        std::vector<std::string> tokens = {"add", "--title", "Dijkstra", "--lang", "cpp"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "add" || !args.hasOption("title") ||
            args.getOption("title") != "Dijkstra" || args.getOption("lang") != "cpp") {
            std::cerr << "[FAIL] Basic subcommand and options parsing failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 2: Options with equals syntax (--key=value)
    {
        std::vector<std::string> tokens = {"list", "--format=markdown", "--tag=graph"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "list" || args.getOption("format") != "markdown" ||
            args.getOption("tag") != "graph") {
            std::cerr << "[FAIL] Equals option parsing failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 3: Boolean flags and short switches
    {
        std::vector<std::string> tokens = {"get", "--title", "BFS", "--code-only", "-v"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (!args.hasFlag("--code-only") || !args.hasFlag("-v") || args.hasFlag("-h")) {
            std::cerr << "[FAIL] Boolean flags parsing failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 4: Positional arguments
    {
        std::vector<std::string> tokens = {"search", "dijkstra", "algorithm"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "search" || args.getPositionals().size() != 2 ||
            args.getPositionals()[0] != "dijkstra" || args.getPositionals()[1] != "algorithm") {
            std::cerr << "[FAIL] Positional arguments parsing failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 5: Fallback default value on getOption
    {
        std::vector<std::string> tokens = {"stats"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getOption("nonexistent", "fallback_val") != "fallback_val") {
            std::cerr << "[FAIL] Fallback default value failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 6: Empty tokens fallback to --help flag
    {
        auto args = snipcli::controller::CliParser::parse({});
        if (!args.hasFlag("--help")) {
            std::cerr << "[FAIL] Empty tokens fallback to --help failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 7: CommandDispatcher registration and execution dispatching
    {
        snipcli::controller::CommandDispatcher dispatcher;
        dispatcher.registerCommand(std::make_unique<MockCommand>("test", "Test command", 42));
        if (!dispatcher.hasCommand("test") || dispatcher.getCommandCount() != 1) {
            std::cerr << "[FAIL] CommandDispatcher registration failed!" << std::endl;
            std::exit(1);
        }
        int code = dispatcher.dispatch("test");
        if (code != 42) {
            std::cerr << "[FAIL] CommandDispatcher dispatch returned unexpected code: " << code
                      << std::endl;
            std::exit(1);
        }
    }

    // Test 8: CommandDispatcher unknown verb returns non-zero error code
    {
        snipcli::controller::CommandDispatcher dispatcher;
        int code = dispatcher.dispatch("unknown_verb");
        if (code == 0) {
            std::cerr << "[FAIL] Unknown verb dispatch unexpectedly succeeded with 0!" << std::endl;
            std::exit(1);
        }
    }

    // Test 9: CommandDispatcher duplicate registration idempotency
    {
        snipcli::controller::CommandDispatcher dispatcher;
        dispatcher.registerCommand(std::make_unique<MockCommand>("add", "Initial add", 0));
        dispatcher.registerCommand(std::make_unique<MockCommand>("add", "Updated add", 0));
        if (dispatcher.getCommandCount() != 1 || dispatcher.getRegisteredCommands().size() != 1) {
            std::cerr << "[FAIL] Duplicate command registration corrupted command order!"
                      << std::endl;
            std::exit(1);
        }
    }

    // Test 10: CommandDispatcher printHelp and printVersion stream rendering
    {
        snipcli::controller::CommandDispatcher dispatcher;
        dispatcher.registerCommand(std::make_unique<MockCommand>("demo", "A demo command", 0));
        std::ostringstream helpStream;
        dispatcher.printHelp(helpStream);
        std::string helpOut = helpStream.str();
        if (helpOut.find("demo") == std::string::npos ||
            helpOut.find("Available Commands:") == std::string::npos) {
            std::cerr << "[FAIL] printHelp stream rendering failed!" << std::endl;
            std::exit(1);
        }

        std::ostringstream verStream;
        dispatcher.printVersion(verStream);
        if (verStream.str().find("SnipCLI version") == std::string::npos) {
            std::cerr << "[FAIL] printVersion stream rendering failed!" << std::endl;
            std::exit(1);
        }
    }

    std::cout << "[PASS] CliParser & CommandDispatcher tests passed. (10/10 Invariants Verified)"
              << std::endl;
}
