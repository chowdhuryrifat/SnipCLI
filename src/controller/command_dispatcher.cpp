#include "snipcli/controller/command_dispatcher.hpp"

#include <iomanip>
#include <iostream>

namespace snipcli::controller {

void CommandDispatcher::registerCommand(std::unique_ptr<ICommand> command) {
    if (!command)
        return;
    std::string name = command->getName();
    m_commands[name] = std::move(command);
    m_order.push_back(name);
}

int CommandDispatcher::dispatch(const std::string& verb) {
    auto it = m_commands.find(verb);
    if (it != m_commands.end()) {
        return it->second->execute();
    }
    std::cerr << "Error: Unknown command '" << verb << "'.\n\n";
    printHelp();
    return 1;
}

bool CommandDispatcher::hasCommand(const std::string& verb) const {
    return m_commands.find(verb) != m_commands.end();
}

void CommandDispatcher::printHelp() const {
    std::cout << "SnipCLI - Algorithmic Snippet Manager for Competitive Programmers\n";
    std::cout << "Usage: snip <command> [options]\n\n";
    std::cout << "Available Commands:\n";

    for (const auto& name : m_order) {
        auto it = m_commands.find(name);
        if (it != m_commands.end()) {
            std::cout << "  " << std::left << std::setw(12) << name << it->second->getDescription()
                      << "\n";
        }
    }

    std::cout << "\nGlobal Options:\n";
    std::cout << "  --help, -h       Display this help documentation\n";
    std::cout << "  --version, -v    Display application version\n";
    std::cout << "  --format <fmt>   Output format ('terminal' or 'markdown')\n";
}

}  // namespace snipcli::controller
