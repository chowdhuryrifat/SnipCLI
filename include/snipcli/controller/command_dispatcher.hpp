#pragma once

#include "snipcli/controller/i_command.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace snipcli::controller {

/**
 * @brief Dispatcher and registry for CLI commands adhering to the Open/Closed Principle.
 */
class CommandDispatcher {
public:
    void registerCommand(std::unique_ptr<ICommand> command);
    [[nodiscard]] int dispatch(const std::string& verb);
    void printHelp() const;
    [[nodiscard]] bool hasCommand(const std::string& verb) const;

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;
    std::vector<std::string> m_order;
};

} // namespace snipcli::controller
