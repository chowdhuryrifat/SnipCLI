#pragma once

#include <string>

namespace snipcli::controller {

/**
 * @brief Command Pattern (GoF): Abstract interface for executable CLI actions.
 *
 * Encapsulates a command request as a standalone object per the GoF Command Pattern.
 * Decouples the CLI invoker/dispatcher from the concrete business operations and
 * repository manipulations. Every CLI verb (add, list, get, delete, search, stats)
 * is encapsulated as an ICommand subclass.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Executes the CLI action encapsulated by the concrete command.
     * @return int Shell exit code (0 for success, non-zero for error).
     */
    virtual int execute() = 0;

    /**
     * @brief Retrieves the primary command verb identifier.
     * @return std::string The unique command name used for CLI dispatching.
     */
    [[nodiscard]] virtual std::string getName() const = 0;

    /**
     * @brief Retrieves a human-readable description of the command's purpose and usage.
     * @return std::string Brief summary description displayed in CLI help screens.
     */
    [[nodiscard]] virtual std::string getDescription() const = 0;
};

}  // namespace snipcli::controller
