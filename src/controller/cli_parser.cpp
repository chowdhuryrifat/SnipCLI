#include "snipcli/controller/cli_parser.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::controller {

namespace {

bool isValueOption(std::string_view opt, std::string_view verb) {
    if (opt == "id" || opt == "title" || opt == "format" || opt == "code" || opt == "file" ||
        opt == "lang" || opt == "language" || opt == "tag" || opt == "tags" || opt == "t" ||
        opt == "l" || opt == "f") {
        return true;
    }
    if (opt == "c") {
        // -c takes a value (--code) unless verb is "get", where it is a flag (--code-only)
        return verb != "get";
    }
    return false;
}

bool isNextTokenAnOptionOrFlag(std::string_view token, std::string_view verb) {
    if (token == "--") {
        return true;
    }
    if (token.rfind("--", 0) == 0) {
        return true;
    }
    if (token == "-h" || token == "-v" || token == "-t" || token == "-l" || token == "-f") {
        return true;
    }
    if (token == "-c" && (verb == "get" || verb.empty())) {
        return true;
    }
    return false;
}

std::string findVerb(const std::vector<std::string>& tokens) {
    bool endOfOptions = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& tok = tokens[i];
        if (!endOfOptions && tok == "--") {
            endOfOptions = true;
            continue;
        }
        if (endOfOptions) {
            return tok;
        }
        if (tok.rfind("--", 0) == 0) {
            std::string body = tok.substr(2);
            size_t eqPos = body.find('=');
            if (eqPos == std::string::npos) {
                if (tok == "--id" || tok == "--title" || tok == "--format" || tok == "--code" ||
                    tok == "--file" || tok == "--lang" || tok == "--language" || tok == "--tag" ||
                    tok == "--tags") {
                    if (i + 1 < tokens.size()) {
                        ++i;
                    }
                }
            }
        } else if (tok == "-t" || tok == "-l" || tok == "-f") {
            if (i + 1 < tokens.size()) {
                ++i;
            }
        } else if (tok == "-h" || tok == "-v") {
            // boolean flag, do not skip
        } else if (tok.rfind("-", 0) == 0 && tok.size() > 1 &&
                   !std::isdigit(static_cast<unsigned char>(tok[1]))) {
            // Other switch
        } else {
            // First non-option positional token is the verb
            return tok;
        }
    }
    return "";
}

void validateFormat(const std::string& val) {
    if (val != "terminal" && val != "markdown") {
        throw common::CommandException("Invalid format '" + val +
                                       "'. Allowed formats are: terminal, markdown.");
    }
}

}  // namespace

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

    std::string verb = findVerb(tokens);
    result.verb = verb;
    bool verbConsumed = false;
    bool endOfOptions = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& arg = tokens[i];

        if (!endOfOptions && arg == "--") {
            endOfOptions = true;
            continue;
        }

        if (endOfOptions) {
            if (!verbConsumed && !verb.empty() && arg == verb) {
                verbConsumed = true;
            } else {
                result.positionals.push_back(arg);
            }
            continue;
        }

        if (arg.rfind("--", 0) == 0) {
            std::string body = arg.substr(2);
            size_t eqPos = body.find('=');
            if (eqPos != std::string::npos) {
                std::string key = body.substr(0, eqPos);
                std::string val = body.substr(eqPos + 1);
                if (isValueOption(key, verb) && val.empty()) {
                    throw common::CommandException("Option '--" + key + "' requires a value.");
                }
                if (key == "format") {
                    validateFormat(val);
                }
                result.options[key] = val;
            } else if (body == "code-only" || body == "help" || body == "version") {
                result.flags.push_back(arg);
            } else if (isValueOption(body, verb)) {
                if (i + 1 >= tokens.size() || isNextTokenAnOptionOrFlag(tokens[i + 1], verb)) {
                    throw common::CommandException("Option '" + arg + "' requires a value.");
                }
                std::string val = tokens[++i];
                if (body == "format") {
                    validateFormat(val);
                }
                result.options[body] = val;
            } else {
                result.flags.push_back(arg);
            }
        } else if (arg.rfind("-", 0) == 0 && arg.size() > 1) {
            size_t eqPos = arg.find('=');
            if (eqPos != std::string::npos) {
                std::string key = arg.substr(1, eqPos - 1);
                std::string val = arg.substr(eqPos + 1);
                if (isValueOption(key, verb) && val.empty()) {
                    throw common::CommandException("Option '-" + key + "' requires a value.");
                }
                result.options[key] = val;
                if (key == "t") {
                    result.options["title"] = val;
                }
                if (key == "l") {
                    result.options["lang"] = val;
                }
                if (key == "f") {
                    result.options["file"] = val;
                }
                if (key == "c") {
                    result.options["code"] = val;
                }
            } else if (arg == "-h" || arg == "-v") {
                result.flags.push_back(arg);
            } else if (arg == "-c" && verb == "get") {
                result.flags.push_back(arg);
            } else if (arg == "-t" || arg == "-l" || arg == "-f" ||
                       (arg == "-c" && verb != "get")) {
                if (i + 1 >= tokens.size() || isNextTokenAnOptionOrFlag(tokens[i + 1], verb)) {
                    throw common::CommandException("Option '" + arg + "' requires a value.");
                }
                std::string val = tokens[++i];
                std::string key = arg.substr(1);
                result.options[key] = val;
                if (key == "t") {
                    result.options["title"] = val;
                }
                if (key == "l") {
                    result.options["lang"] = val;
                }
                if (key == "f") {
                    result.options["file"] = val;
                }
                if (key == "c") {
                    result.options["code"] = val;
                }
            } else if (std::isdigit(static_cast<unsigned char>(arg[1]))) {
                if (!verbConsumed && !verb.empty() && arg == verb) {
                    verbConsumed = true;
                } else {
                    result.positionals.push_back(arg);
                }
            } else {
                result.flags.push_back(arg);
            }
        } else {
            if (!verbConsumed && !verb.empty() && arg == verb) {
                verbConsumed = true;
            } else {
                result.positionals.push_back(arg);
            }
        }
    }

    return result;
}

}  // namespace snipcli::controller
