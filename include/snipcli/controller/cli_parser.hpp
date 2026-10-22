#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace snipcli::controller {

/**
 * @brief Structured representation of parsed CLI command-line inputs.
 *
 * Encapsulates the extracted command verb, named options (e.g., --title "Dijkstra"),
 * standalone flags/switches (e.g., --help, -v), and positional arguments.
 * Serves as an immutable/queried DTO passed into ICommand subclasses.
 */
struct ParsedArgs {
    /// Primary CLI action verb (e.g., "add", "list", "get", "delete", "search", "stats").
    std::string verb;

    /// Key-value mapping of named options (e.g., "title" -> "Dijkstra", "format" -> "markdown").
    std::unordered_map<std::string, std::string> options;

    /// Collection of standalone boolean switches or flags (e.g., "--help", "-v", "--code-only").
    std::vector<std::string> flags;

    /// Collection of positional command arguments not attached to an option flag.
    std::vector<std::string> positionals;

    /**
     * @brief Checks whether a specific boolean flag or switch was present in the command arguments.
     * @param flag The flag string to check (e.g., "--code-only", "-v", "--help").
     * @return bool True if the flag was supplied; false otherwise.
     */
    [[nodiscard]] bool hasFlag(const std::string& flag) const;

    /**
     * @brief Checks whether a named option key was supplied.
     * @param key The option key name without leading dashes (e.g., "title", "lang").
     * @return bool True if the option exists; false otherwise.
     */
    [[nodiscard]] bool hasOption(const std::string& key) const;

    /**
     * @brief Retrieves the string value associated with a named option key.
     * @param key The option key name without leading dashes.
     * @param defaultValue The fallback string returned if the option key was not supplied.
     * @return std::string The option value if present, or defaultValue.
     */
    [[nodiscard]] std::string getOption(const std::string& key,
                                        const std::string& defaultValue = "") const;

    /**
     * @brief Retrieves the primary command verb.
     * @return const std::string& Reference to the verb string.
     */
    [[nodiscard]] const std::string& getVerb() const noexcept;

    /**
     * @brief Retrieves the collection of positional arguments.
     * @return const std::vector<std::string>& Reference to positionals vector.
     */
    [[nodiscard]] const std::vector<std::string>& getPositionals() const noexcept;
};

/**
 * @brief Command line argument parser converting argv tokens into structured ParsedArgs.
 *
 * Handles CLI tokenization including:
 * - Subcommand verb detection (first non-dashed argument).
 * - Key-value options with space separation (`--flag value`).
 * - Key-value options with equals delimiter (`--flag=value`).
 * - Boolean switches and short flags (`-h`, `-v`, `--code-only`).
 * - Positional arguments.
 * - Automatic fallback to `--help` when zero arguments are provided.
 */
class CliParser {
public:
    /**
     * @brief Parses standard command-line argument vectors into a structured ParsedArgs DTO.
     * @param argc Argument count from main().
     * @param argv Array of null-terminated argument strings from main().
     * @return ParsedArgs The structured result containing verb, options, flags, and positionals.
     */
    [[nodiscard]] static ParsedArgs parse(int argc, char* const argv[]);

    /**
     * @brief Helper overload to parse a collection of string tokens.
     * @param tokens List of argument strings excluding the binary name.
     * @return ParsedArgs The structured result.
     */
    [[nodiscard]] static ParsedArgs parse(const std::vector<std::string>& tokens);
};

}  // namespace snipcli::controller
