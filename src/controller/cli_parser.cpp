#include "snipcli/controller/cli_parser.hpp"

#include <algorithm>

namespace snipcli::controller {

bool ParsedArgs::hasFlag(const std::string& flag) const {
    return std::find(flags.begin(), flags.end(), flag) != flags.end();
}

bool ParsedArgs::hasOption(const std::string& key) const {
    return options.find(key) != options.end();
}

std::string ParsedArgs::getOption(const std::string& key, const std::string& defaultValue) const {
    auto it = options.find(key);
    if (it != options.end()) {
        return it->second;
    }
    return defaultValue;
}

const std::string& ParsedArgs::getVerb() const noexcept {
    return verb;
}

const std::vector<std::string>& ParsedArgs::getPositionals() const noexcept {
    return positionals;
}

ParsedArgs CliParser::parse(int argc, char* const argv[]) {
    if (argc <= 1 || argv == nullptr) {
        ParsedArgs fallback;
        fallback.flags.emplace_back("--help");
        return fallback;
    }

    std::vector<std::string> tokens;
    tokens.reserve(static_cast<size_t>(argc - 1));
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr) {
            tokens.emplace_back(argv[i]);
        }
    }

    return parse(tokens);
}

ParsedArgs CliParser::parse(const std::vector<std::string>& tokens) {
    ParsedArgs result;
    if (tokens.empty()) {
        result.flags.emplace_back("--help");
        return result;
    }

    size_t i = 0;
    // Check if first token is a subcommand verb (does not start with '-')
    const std::string& first = tokens[0];
    if (!first.empty() && first[0] != '-') {
        result.verb = first;
        i = 1;
    }

    while (i < tokens.size()) {
        const std::string& arg = tokens[i];
        if (arg.rfind("--", 0) == 0) {
            std::string body = arg.substr(2);
            size_t eqPos = body.find('=');
            if (eqPos != std::string::npos) {
                // Syntax: --key=value
                std::string key = body.substr(0, eqPos);
                std::string val = body.substr(eqPos + 1);
                result.options[key] = val;
            } else if (i + 1 < tokens.size() &&
                       (tokens[i + 1].empty() || tokens[i + 1][0] != '-')) {
                // Syntax: --key value
                result.options[body] = tokens[++i];
            } else {
                // Standalone boolean flag: --flag
                result.flags.push_back(arg);
            }
        } else if (arg.rfind("-", 0) == 0) {
            // Short switch: -h, -v, etc.
            result.flags.push_back(arg);
        } else {
            // Positional argument
            result.positionals.push_back(arg);
        }
        ++i;
    }

    return result;
}

}  // namespace snipcli::controller
