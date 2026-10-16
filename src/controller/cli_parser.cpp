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

ParsedArgs CliParser::parse(int argc, char* argv[]) {
    ParsedArgs result;
    if (argc <= 1) {
        result.flags.emplace_back("--help");
        return result;
    }

    int i = 1;
    // Check if first arg is a verb (doesn't start with '-')
    std::string first = argv[i];
    if (!first.empty() && first[0] != '-') {
        result.verb = first;
        i++;
    }

    while (i < argc) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string key = arg.substr(2);
            // Check if next arg is value
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                result.options[key] = argv[++i];
            } else {
                result.flags.push_back(arg);
            }
        } else if (arg.rfind("-", 0) == 0) {
            result.flags.push_back(arg);
        } else {
            result.positionals.push_back(arg);
        }
        i++;
    }

    return result;
}

}  // namespace snipcli::controller
