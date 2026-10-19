#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "snipcli/common/exceptions.hpp"
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

    // Test 11: Standalone boolean flags never swallow subsequent positional arguments
    {
        std::vector<std::string> tokens = {"get", "--code-only", "5"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "get" || !args.hasFlag("--code-only") ||
            args.hasOption("code-only") || args.getPositionals().size() != 1 ||
            args.getPositionals()[0] != "5") {
            std::cerr << "[FAIL] Standalone flag swallowed positional argument!" << std::endl;
            std::exit(1);
        }
    }

    // Test 12: Negative numeric option values (--id -5) and negative positionals
    {
        std::vector<std::string> tokens = {"get", "--id", "-5"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "get" || !args.hasOption("id") || args.getOption("id") != "-5") {
            std::cerr << "[FAIL] Spaced negative numeric option value parsing failed!" << std::endl;
            std::exit(1);
        }

        std::vector<std::string> posTokens = {"delete", "-5"};
        auto posArgs = snipcli::controller::CliParser::parse(posTokens);
        if (posArgs.getVerb() != "delete" || posArgs.getPositionals().size() != 1 ||
            posArgs.getPositionals()[0] != "-5") {
            std::cerr << "[FAIL] Negative numeric positional argument parsing failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 13: Global options before verb (Finding B3)
    {
        std::vector<std::string> tokens = {"--format", "markdown", "list"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "list" || args.getOption("format") != "markdown" ||
            !args.getPositionals().empty()) {
            std::cerr << "[FAIL] Global options before verb failed!" << std::endl;
            std::exit(1);
        }

        std::vector<std::string> flagTokens = {"-v", "list"};
        auto flagArgs = snipcli::controller::CliParser::parse(flagTokens);
        if (flagArgs.getVerb() != "list" || !flagArgs.hasFlag("-v") ||
            !flagArgs.getPositionals().empty()) {
            std::cerr << "[FAIL] Global flag before verb failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 14: Valueless option handling throws CommandException (Finding B2)
    {
        // Option followed by another option: --id --title X
        try {
            static_cast<void>(
                snipcli::controller::CliParser::parse({"delete", "--id", "--title", "X"}));
            std::cerr << "[FAIL] Valueless --id followed by --title did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            std::string msg = ex.what();
            if (msg.find("Option '--id' requires a value") == std::string::npos) {
                std::cerr << "[FAIL] Unexpected error message: " << msg << std::endl;
                std::exit(1);
            }
        }

        // Option at end of line without value: --id
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"delete", "--id"}));
            std::cerr << "[FAIL] Valueless --id at end of arguments did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            // Expected
        }

        // Valueless --code followed by flag -h: --code -h
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"add", "--code", "-h"}));
            std::cerr << "[FAIL] Valueless --code followed by -h did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            // Expected
        }

        // Valueless short option: -t
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"add", "-t"}));
            std::cerr << "[FAIL] Valueless -t did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            // Expected
        }

        // Valueless -c when verb is add: add -c
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"add", "-c"}));
            std::cerr << "[FAIL] Valueless -c in add did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            // Expected
        }
    }

    // Test 15: Invalid --format validation (Finding B4)
    {
        try {
            static_cast<void>(
                snipcli::controller::CliParser::parse({"--format", "markdwon", "list"}));
            std::cerr << "[FAIL] Invalid --format markdwon did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            std::string msg = ex.what();
            if (msg.find("Invalid format 'markdwon'") == std::string::npos) {
                std::cerr << "[FAIL] Unexpected format error message: " << msg << std::endl;
                std::exit(1);
            }
        }

        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"list", "--format=html"}));
            std::cerr << "[FAIL] Invalid --format=html did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            // Expected
        }
    }

    // Test 16: Values starting with dash accepted for options requiring values (Finding B8)
    {
        std::vector<std::string> tokens = {"add", "--title", "DashSnippet", "--code", "-x;"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "add" || args.getOption("title") != "DashSnippet" ||
            args.getOption("code") != "-x;") {
            std::cerr << "[FAIL] Value starting with dash '-x;' was not parsed as option value!"
                      << std::endl;
            std::exit(1);
        }

        std::vector<std::string> shortTokens = {"add", "-t", "ShortDash", "-c", "-y;"};
        auto shortArgs = snipcli::controller::CliParser::parse(shortTokens);
        if (shortArgs.getVerb() != "add" || shortArgs.getOption("title") != "ShortDash" ||
            shortArgs.getOption("code") != "-y;") {
            std::cerr << "[FAIL] Short option -c with value '-y;' failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 17: Standard end-of-options delimiter (--) (Finding B8)
    {
        std::vector<std::string> tokens = {"delete", "--", "-5"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "delete" || args.getPositionals().size() != 1 ||
            args.getPositionals()[0] != "-5") {
            std::cerr << "[FAIL] End-of-options delimiter -- handling failed!" << std::endl;
            std::exit(1);
        }

        std::vector<std::string> multiTokens = {"delete", "--", "Binary", "Search"};
        auto multiArgs = snipcli::controller::CliParser::parse(multiTokens);
        if (multiArgs.getPositionals().size() != 2 || multiArgs.getPositionals()[0] != "Binary" ||
            multiArgs.getPositionals()[1] != "Search") {
            std::cerr << "[FAIL] End-of-options delimiter with multiple positionals failed!"
                      << std::endl;
            std::exit(1);
        }
    }

    // Test 18: Short option aliases (-t, -l, -f, -c)
    {
        std::vector<std::string> tokens = {"add", "-t", "Quick", "-l", "python", "-f", "quick.py"};
        auto args = snipcli::controller::CliParser::parse(tokens);
        if (args.getVerb() != "add" || args.getOption("title") != "Quick" ||
            args.getOption("lang") != "python" || args.getOption("file") != "quick.py") {
            std::cerr << "[FAIL] Short option aliases (-t, -l, -f) failed!" << std::endl;
            std::exit(1);
        }

        // -c in get command is standalone --code-only flag
        std::vector<std::string> getTokens = {"get", "-c", "5"};
        auto getArgs = snipcli::controller::CliParser::parse(getTokens);
        if (getArgs.getVerb() != "get" || !getArgs.hasFlag("-c") ||
            getArgs.getPositionals().size() != 1 || getArgs.getPositionals()[0] != "5") {
            std::cerr << "[FAIL] Short switch -c in get command failed!" << std::endl;
            std::exit(1);
        }
    }

    // Test 19: Unknown long option rejection (Review Finding 1)
    {
        try {
            static_cast<void>(
                snipcli::controller::CliParser::parse({"delete", "--titel", "Binary"}));
            std::cerr << "[FAIL] Unknown long option --titel did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            std::string msg = ex.what();
            if (msg.find("Unknown option '--titel'") == std::string::npos) {
                std::cerr << "[FAIL] Unexpected unknown option error message: " << msg << std::endl;
                std::exit(1);
            }
        }
    }

    // Test 20: Unknown short switch rejection (Review Finding 1)
    {
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"add", "-x"}));
            std::cerr << "[FAIL] Unknown short switch -x did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            std::string msg = ex.what();
            if (msg.find("Unknown option '-x'") == std::string::npos) {
                std::cerr << "[FAIL] Unexpected unknown switch error message: " << msg << std::endl;
                std::exit(1);
            }
        }
    }

    // Test 21: Unknown option with equals syntax rejection (Review Finding 1)
    {
        try {
            static_cast<void>(snipcli::controller::CliParser::parse({"list", "--bogus=123"}));
            std::cerr << "[FAIL] Unknown option with equals syntax did not throw!" << std::endl;
            std::exit(1);
        } catch (const snipcli::common::CommandException& ex) {
            std::string msg = ex.what();
            if (msg.find("Unknown option '--bogus'") == std::string::npos) {
                std::cerr << "[FAIL] Unexpected unknown equals error message: " << msg << std::endl;
                std::exit(1);
            }
        }
    }

    std::cout << "[PASS] CliParser & CommandDispatcher tests passed. (21/21 Invariants Verified)"
              << std::endl;
}
