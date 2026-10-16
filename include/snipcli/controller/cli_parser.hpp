#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace snipcli::controller {

/**
 * @brief Structured representation of parsed CLI inputs.
 */
struct ParsedArgs {
    std::string verb;
    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> flags;
    std::vector<std::string> positionals;

    [[nodiscard]] bool hasFlag(const std::string& flag) const;
    [[nodiscard]] bool hasOption(const std::string& key) const;
    [[nodiscard]] std::string getOption(const std::string& key,
                                        const std::string& defaultValue = "") const;
};

/**
 * @brief Command line argument parser converting argv to structured ParsedArgs.
 */
class CliParser {
public:
    [[nodiscard]] static ParsedArgs parse(int argc, char* argv[]);
};

}  // namespace snipcli::controller
