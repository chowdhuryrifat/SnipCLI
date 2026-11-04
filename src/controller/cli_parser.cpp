#include "snipcli/controller/cli_parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/common/string_utils.hpp"

namespace snipcli::controller {

namespace {

/**
 * One accepted spelling of a CLI option. Aliases share a `canonical` key, which is the key
 * stored in ParsedArgs::options (e.g. -t, --title -> "title"; -l, --lang, --language -> "lang").
 * An empty `verbs` list means the option is global (accepted by every command).
 */
struct OptionSpec {
    std::string_view name;
    bool isShort;
    bool takesValue;
    std::string_view canonical;
    std::array<std::string_view, 4> verbs;

    [[nodiscard]] bool isGlobal() const noexcept { return verbs[0].empty(); }

    [[nodiscard]] bool supports(std::string_view verb) const noexcept {
        if (isGlobal()) {
            return true;
        }
        // Unused trailing slots are empty, so an empty verb must never match them.
        return !verb.empty() && std::find(verbs.begin(), verbs.end(), verb) != verbs.end();
    }
};

// clang-format off
constexpr OptionSpec kOptionSpecs[] = {
    // Global switches and options
    {"help",      false, false, "help",      {}},
    {"h",         true,  false, "help",      {}},
    {"version",   false, false, "version",   {}},
    {"v",         true,  false, "version",   {}},
    {"format",    false, true,  "format",    {}},

    // Snippet selection and metadata
    {"title",     false, true,  "title",     {"add", "get", "delete", "search"}},
    {"t",         true,  true,  "title",     {"add", "get", "delete", "search"}},
    {"lang",      false, true,  "lang",      {"add", "list", "search"}},
    {"language",  false, true,  "lang",      {"add", "list", "search"}},
    {"l",         true,  true,  "lang",      {"add", "list", "search"}},
    {"tag",       false, true,  "tag",       {"add", "list", "search"}},
    {"tags",      false, true,  "tag",       {"add", "list", "search"}},
    {"id",        false, true,  "id",        {"get", "delete"}},

    // Source code input (add)
    {"code",      false, true,  "code",      {"add"}},
    {"file",      false, true,  "file",      {"add"}},
    {"f",         true,  true,  "file",      {"add"}},

    // Output control (get)
    {"code-only", false, false, "code-only", {"get"}},

    // Dual-personality -c: --code (takes a value) for add, --code-only (boolean) for get
    {"c",         true,  true,  "code",      {"add"}},
    {"c",         true,  false, "code-only", {"get"}},
};
// clang-format on

/**
 * Finds the spec for an option spelling. When several specs share a spelling (-c), the one
 * supporting @p verb wins; otherwise the first match is returned so that callers can report
 * "not supported by command".
 */
const OptionSpec* findSpec(std::string_view name, bool isShort, std::string_view verb) noexcept {
    const OptionSpec* firstMatch = nullptr;
    for (const auto& spec : kOptionSpecs) {
        if (spec.name != name || spec.isShort != isShort) {
            continue;
        }
        if (spec.supports(verb)) {
            return &spec;
        }
        if (firstMatch == nullptr) {
            firstMatch = &spec;
        }
    }
    return firstMatch;
}

bool startsWith(std::string_view s, std::string_view prefix) noexcept {
    return s.substr(0, prefix.size()) == prefix;
}

bool isDigit(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isKnownVerb(std::string_view v) noexcept {
    return v == "add" || v == "list" || v == "get" || v == "delete" || v == "search" ||
           v == "stats" || v == "help";
}

/// True if @p token would be parsed as an option/flag rather than as a value.
bool looksLikeOption(std::string_view token, std::string_view verb) noexcept {
    if (startsWith(token, "--")) {
        return true;  // includes the "--" end-of-options marker
    }
    if (token.size() < 2 || token[0] != '-') {
        return false;
    }
    const std::string_view name = token.substr(1);
    if (name == "c") {
        // -c is a boolean switch for `get` (and before the verb is known), so it can't be a value.
        return verb == "get" || verb.empty();
    }
    return findSpec(name, true, verb) != nullptr;
}

[[noreturn]] void throwUnsupported(std::string_view display, std::string_view verb) {
    if (verb.empty()) {
        throw common::CommandException("Option '" + std::string(display) +
                                       "' is not supported without a command.");
    }
    throw common::CommandException("Option '" + std::string(display) +
                                   "' is not supported by command '" + std::string(verb) + "'.");
}

/**
 * Validates a --format value and returns its canonical name. Matching is case-insensitive and
 * `md` is accepted as an alias for `markdown`, so commands only ever see "terminal"/"markdown".
 */
std::string canonicalFormat(const std::string& val) {
    const std::string lower = common::toLower(val);
    if (lower == "terminal") {
        return "terminal";
    }
    if (lower == "markdown" || lower == "md") {
        return "markdown";
    }
    throw common::CommandException("Invalid format '" + val +
                                   "'. Allowed formats are: terminal, markdown (alias: md).");
}

/// Validates and stores an option value under its canonical key.
void storeValue(ParsedArgs& result, const OptionSpec& spec, std::string value) {
    if (spec.canonical == "format") {
        value = canonicalFormat(value);
    }
    result.options[std::string(spec.canonical)] = std::move(value);
}

/**
 * Handles an inline-valued option (`--key=value` or `-k=value`).
 * @param prefix "--" or "-", used to render the option name in error messages.
 */
void parseInlineValue(ParsedArgs& result, std::string_view prefix, std::string_view key,
                      std::string value, bool isShort, std::string_view verb) {
    const std::string display = std::string(prefix) + std::string(key);
    const OptionSpec* spec = findSpec(key, isShort, verb);
    if (spec == nullptr) {
        throw common::CommandException("Unknown option '" + display + "'.");
    }
    if (!spec->takesValue) {
        throw common::CommandException("Option '" + display +
                                       "' is a boolean flag and does not accept a value.");
    }
    if (!spec->supports(verb)) {
        throwUnsupported(display, verb);
    }
    if (value.empty()) {
        throw common::CommandException("Option '" + display + "' requires a value.");
    }
    storeValue(result, *spec, std::move(value));
}

/**
 * Handles a bare option token (`--key` or `-k`), consuming the following token as its value
 * when the option takes one. Advances @p i past any consumed value.
 */
void parseBareOption(ParsedArgs& result, const std::vector<std::string>& tokens, size_t& i,
                     std::string_view name, bool isShort, std::string_view verb) {
    const std::string& arg = tokens[i];
    const OptionSpec* spec = findSpec(name, isShort, verb);
    if (spec == nullptr) {
        throw common::CommandException("Unknown option '" + arg + "'.");
    }
    if (!spec->supports(verb)) {
        throwUnsupported(arg, verb);
    }
    if (!spec->takesValue) {
        result.flags.push_back(arg);
        return;
    }
    if (i + 1 >= tokens.size() || looksLikeOption(tokens[i + 1], verb)) {
        throw common::CommandException("Option '" + arg + "' requires a value.");
    }
    storeValue(result, *spec, tokens[++i]);
}

/**
 * Locates the command verb: the first token that is neither an option nor an option's value.
 * Runs before the main pass so that per-command option rules can be applied.
 */
std::string findVerb(const std::vector<std::string>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& tok = tokens[i];
        if (tok == "--") {
            return i + 1 < tokens.size() ? tokens[i + 1] : std::string();
        }
        if (startsWith(tok, "--")) {
            const std::string_view body = std::string_view(tok).substr(2);
            if (body.find('=') == std::string_view::npos) {
                const OptionSpec* spec = findSpec(body, false, "");
                if (spec != nullptr && spec->takesValue && i + 1 < tokens.size()) {
                    ++i;
                }
            }
        } else if (tok == "-c") {
            // Ambiguous until the verb is known: skip a following value unless it's a verb.
            if (i + 1 < tokens.size() && !isKnownVerb(tokens[i + 1])) {
                ++i;
            }
        } else if (tok.size() > 1 && tok[0] == '-' && !isDigit(tok[1])) {
            const OptionSpec* spec = findSpec(std::string_view(tok).substr(1), true, "");
            if (spec != nullptr && spec->takesValue && i + 1 < tokens.size()) {
                ++i;
            }
        } else {
            return tok;
        }
    }
    return "";
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
    return it != options.end() ? it->second : defaultValue;
}

const std::string& ParsedArgs::getVerb() const noexcept {
    return verb;
}

const std::vector<std::string>& ParsedArgs::getPositionals() const noexcept {
    return positionals;
}

ParsedArgs CliParser::parse(int argc, char* const argv[]) {
    std::vector<std::string> tokens;
    if (argc > 1 && argv != nullptr) {
        tokens.reserve(static_cast<size_t>(argc - 1));
        for (int i = 1; i < argc; ++i) {
            if (argv[i] != nullptr) {
                tokens.emplace_back(argv[i]);
            }
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

    const std::string verb = findVerb(tokens);
    result.verb = verb;
    bool verbConsumed = false;
    bool endOfOptions = false;

    auto addPositional = [&](const std::string& arg) {
        if (!verbConsumed && !verb.empty() && arg == verb) {
            verbConsumed = true;
        } else {
            result.positionals.push_back(arg);
        }
    };

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& arg = tokens[i];

        if (endOfOptions) {
            addPositional(arg);
        } else if (arg == "--") {
            endOfOptions = true;
        } else if (startsWith(arg, "--")) {
            const std::string_view body = std::string_view(arg).substr(2);
            const size_t eq = body.find('=');
            if (eq != std::string_view::npos) {
                parseInlineValue(result, "--", body.substr(0, eq), std::string(body.substr(eq + 1)),
                                 false, verb);
            } else {
                parseBareOption(result, tokens, i, body, false, verb);
            }
        } else if (arg.size() > 1 && arg[0] == '-') {
            const std::string_view body = std::string_view(arg).substr(1);
            const size_t eq = body.find('=');
            if (eq != std::string_view::npos) {
                parseInlineValue(result, "-", body.substr(0, eq), std::string(body.substr(eq + 1)),
                                 true, verb);
            } else if (isDigit(body[0])) {
                addPositional(arg);  // negative number, e.g. `snip get -- -5` or `snip get -5`
            } else {
                parseBareOption(result, tokens, i, body, true, verb);
            }
        } else {
            addPositional(arg);
        }
    }

    return result;
}

}  // namespace snipcli::controller
