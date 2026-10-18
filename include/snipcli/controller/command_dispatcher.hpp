#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "snipcli/controller/i_command.hpp"

namespace snipcli::controller {

/**
 * @brief Dispatcher and registry for CLI commands adhering to the Open/Closed Principle (OCP).
 *
 * Encapsulates the command execution registry. Decouples CLI routing from individual command
 * implementations. New commands can be registered without modifying the dispatcher's core logic.
 * Manages unique ownership of ICommand instances using std::unique_ptr.
 */
class CommandDispatcher {
public:
    /**
     * @brief Registers an executable command instance into the command catalog.
     *
     * If a command with the same name was previously registered, the existing instance is
     * replaced while preserving command ordering. Null pointers are safely ignored.
     *
     * @param command Unique pointer transferring ownership of the ICommand implementation.
     */
    void registerCommand(std::unique_ptr<ICommand> command);

    /**
     * @brief Dispatches execution to the command associated with the provided CLI verb.
     * @param verb The command verb to execute (e.g., "add", "list", "get").
     * @return int Shell exit code returned by the executed command (0 for success, non-zero for
     * error).
     */
    [[nodiscard]] int dispatch(const std::string& verb);

    /**
     * @brief Formats and prints the global CLI help documentation to the specified output stream.
     * @param os Target output stream (defaults to std::cout).
     */
    void printHelp(std::ostream& os = std::cout) const;

    /**
     * @brief Formats and prints the application version banner to the specified output stream.
     * @param os Target output stream (defaults to std::cout).
     */
    void printVersion(std::ostream& os = std::cout) const;

    /**
     * @brief Checks whether a command corresponding to the specified verb is registered.
     * @param verb The command name to query.
     * @return bool True if registered; false otherwise.
     */
    [[nodiscard]] bool hasCommand(const std::string& verb) const noexcept;

    /**
     * @brief Retrieves the total number of currently registered commands.
     * @return size_t Count of registered commands.
     */
    [[nodiscard]] size_t getCommandCount() const noexcept;

    /**
     * @brief Retrieves the list of registered command names in their order of registration.
     * @return std::vector<std::string> Ordered list of command verb identifiers.
     */
    [[nodiscard]] std::vector<std::string> getRegisteredCommands() const;

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands;
    std::vector<std::string> m_order;
};

}  // namespace snipcli::controller
