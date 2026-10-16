#pragma once

#include <string>

namespace snipcli::controller {

/**
 * @brief Command Pattern (GoF): Abstract interface for executable CLI actions.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Executes the CLI action.
     * @return int Shell exit code (0 for success, non-zero for error).
     */
    virtual int execute() = 0;

    [[nodiscard]] virtual std::string getName() const = 0;
    [[nodiscard]] virtual std::string getDescription() const = 0;
};

} // namespace snipcli::controller
