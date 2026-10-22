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

void CommandDispatcher::printHelp(std::ostream& os) const {
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
    os << "  --format <fmt>   Output format ('terminal' or 'markdown')\n";
}

void CommandDispatcher::printVersion(std::ostream& os) const {
    os << "SnipCLI version " << SNIPCLI_VERSION_STRING << " (Modern C++17 MVC Architecture)\n";
}

}  // namespace snipcli::controller
