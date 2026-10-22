#include "snipcli/controller/command_dispatcher.hpp"

#include <iomanip>
#include <iostream>

#include "version.hpp"

namespace snipcli::controller {

void CommandDispatcher::registerCommand(std::unique_ptr<ICommand> command) {
    if (!command) {
        return;
    }
    std::string name = command->getName();
    if (m_commands.find(name) == m_commands.end()) {
        m_order.push_back(name);
    }
    m_commands[name] = std::move(command);
}

int CommandDispatcher::dispatch(const std::string& verb) {
    auto it = m_commands.find(verb);
    if (it != m_commands.end()) {
        return it->second->execute();
    }
    std::cerr << "Error: Unknown command '" << verb << "'.\n\n";
    printHelp(std::cerr);
    return 1;
}

bool CommandDispatcher::hasCommand(const std::string& verb) const noexcept {
    return m_commands.find(verb) != m_commands.end();
}

size_t CommandDispatcher::getCommandCount() const noexcept {
    return m_commands.size();
}

const std::vector<std::string>& CommandDispatcher::getRegisteredCommands() const noexcept {
    return m_order;
}

void CommandDispatcher::printHelp(std::ostream& os, const std::string& specificCommand) const {
    if (specificCommand == "add") {
        os << "SnipCLI - add command\n";
        os << "Usage: snip add --title <title> --lang <lang> [--code <code> | --file <file>] [--tags <tags>]\n\n";
        os << "Create and persist a new code snippet in the repository.\n\n";
        os << "Options:\n";
        os << "  -t, --title <title>          Unique snippet title (alphanumeric, spaces, hyphens, underscores)\n";
        os << "  -l, --lang, --language <l>   Programming language (e.g. cpp, python, rust, go)\n";
        os << "  -c, --code <code>            Source code content as an inline string (use --code=... if leading dashes)\n";
        os << "  -f, --file <file>            Path to file containing source code to import\n";
        os << "  --tags, --tag <tags>         Comma-separated category tags (e.g. \"graph,shortest_path\")\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Requirements:\n";
        os << "  • --title and --lang are required.\n";
        os << "  • Exactly one of --code or --file must be specified.\n";
        os << "  • Snippet titles must be unique across the repository.\n\n";
        os << "Examples:\n";
        os << "  snip add --title \"Dijkstra\" --lang cpp --file ./dijkstra.cpp --tags \"graph,algo\"\n";
        os << "  snip add --title \"QuickSort\" --lang python --code \"def qsort(a): return a\"\n";
        return;
    }

    if (specificCommand == "list") {
        os << "SnipCLI - list command\n";
        os << "Usage: snip list [options]\n\n";
        os << "List all stored code snippets in tabular format with optional filtering.\n\n";
        os << "Options:\n";
        os << "  -l, --lang, --language <l>   Filter snippets by programming language\n";
        os << "  --tag, --tags <tag>          Filter snippets by category tag\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Examples:\n";
        os << "  snip list\n";
        os << "  snip list --lang cpp\n";
        os << "  snip list --tag graph --format markdown\n";
        return;
    }

    if (specificCommand == "get") {
        os << "SnipCLI - get command\n";
        os << "Usage: snip get [--id <id> | --title <title> | <query>] [options]\n\n";
        os << "Retrieve and display a code snippet by ID or title.\n\n";
        os << "Options:\n";
        os << "  --id <id>                    Exact numeric identifier of the snippet\n";
        os << "  -t, --title <title>          Exact title of the snippet\n";
        os << "  -c, --code-only              Output raw source code byte-for-byte (pipe-friendly, no headers)\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Notes:\n";
        os << "  • Specify either --id or --title, not both.\n";
        os << "  • With --code-only, raw source code is emitted without metadata, perfect for clipboard pipes.\n\n";
        os << "Examples:\n";
        os << "  snip get --id 1\n";
        os << "  snip get --title \"Dijkstra\"\n";
        os << "  snip get --title \"Dijkstra\" --code-only | pbcopy\n";
        os << "  snip get --id 1 --code-only > solution.cpp\n";
        return;
    }

    if (specificCommand == "delete") {
        os << "SnipCLI - delete command\n";
        os << "Usage: snip delete [--id <id> | --title <title> | <query>] [options]\n\n";
        os << "Delete a snippet from the repository by ID or title.\n\n";
        os << "Options:\n";
        os << "  --id <id>                    Exact numeric identifier of the snippet to delete\n";
        os << "  -t, --title <title>          Exact title of the snippet to delete\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Notes:\n";
        os << "  • Specify either --id or --title, not both.\n\n";
        os << "Examples:\n";
        os << "  snip delete --id 1\n";
        os << "  snip delete --title \"Dijkstra\"\n";
        return;
    }

    if (specificCommand == "search") {
        os << "SnipCLI - search command\n";
        os << "Usage: snip search [<query>] [options]\n\n";
        os << "Search snippets by query substring, category tag, or language using the GoF Strategy Pattern.\n\n";
        os << "Options:\n";
        os << "  -t, --title <query>          Search by case-insensitive title substring\n";
        os << "  --tag, --tags <tag>          Search by exact category tag match\n";
        os << "  -l, --lang, --language <l>   Search by exact programming language\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Notes:\n";
        os << "  • Positional queries (e.g. 'snip search dijkstra') perform case-insensitive title search.\n\n";
        os << "Examples:\n";
        os << "  snip search \"dijkstra\"\n";
        os << "  snip search --tag \"graph\"\n";
        os << "  snip search --lang \"python\"\n";
        os << "  snip search --tag \"dp\" --format markdown\n";
        return;
    }

    if (specificCommand == "stats") {
        os << "SnipCLI - stats command\n";
        os << "Usage: snip stats [options]\n\n";
        os << "Display code metrics and repository telemetry across all snippets.\n\n";
        os << "Options:\n";
        os << "  --format <fmt>               Output format ('terminal' or 'markdown')\n\n";
        os << "Metrics Reported:\n";
        os << "  • Total stored snippet volume\n";
        os << "  • Total lines of code across all snippets\n";
        os << "  • Language distribution breakdown sorted by snippet count\n\n";
        os << "Examples:\n";
        os << "  snip stats\n";
        os << "  snip stats --format markdown\n";
        return;
    }

    if (!specificCommand.empty()) {
        os << "Error: Unknown command '" << specificCommand << "' for help.\n\n";
    }

    os << "SnipCLI - Algorithmic Snippet Manager for Competitive Programmers\n";
    os << "Usage: snip <command> [options]\n\n";
    os << "Available Commands:\n";

    for (const auto& name : m_order) {
        auto it = m_commands.find(name);
        if (it != m_commands.end()) {
            os << "  " << std::left << std::setw(12) << name << it->second->getDescription()
               << "\n";
        }
    }

    os << "\nGlobal Options:\n";
    os << "  --help, -h       Display this help documentation\n";
    os << "  --version, -v    Display application version\n";
    os << "  --format <fmt>   Output format ('terminal' or 'markdown')\n\n";

    os << "Common Examples:\n";
    os << "  snip add --title \"Dijkstra\" --lang cpp --file ./dijkstra.cpp --tags \"graph,algo\"\n";
    os << "  snip list --lang cpp\n";
    os << "  snip get --title \"Dijkstra\" --code-only | pbcopy\n";
    os << "  snip search --tag graph\n";
    os << "  snip stats\n\n";

    os << "Environment Variables:\n";
    os << "  SNIPCLI_STORAGE_PATH     Custom storage file location (default: ~/.snipcli/snippets.json)\n";
    os << "  SNIPCLI_LOCK_TIMEOUT_MS  Storage lock acquisition timeout in ms [1-600000] (default: 10000)\n";
    os << "  NO_COLOR                 When non-empty, suppresses ANSI terminal color escapes\n\n";

    os << "Run 'snip <command> --help' or 'snip help <command>' for detailed command options.\n";
}

void CommandDispatcher::printVersion(std::ostream& os) const {
    os << "SnipCLI version " << SNIPCLI_VERSION_STRING << " (Modern C++17 MVC Architecture)\n";
}

}  // namespace snipcli::controller
