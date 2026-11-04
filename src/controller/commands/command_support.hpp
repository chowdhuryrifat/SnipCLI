#pragma once

// Internal helpers shared by concrete commands. Not part of the public include/ API.

#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/common/string_utils.hpp"
#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands::detail {

/// Exit code for domain errors (validation, storage, bad arguments).
inline constexpr int kExitError = 1;
/// Exit code for unexpected non-domain exceptions.
inline constexpr int kExitUnexpected = 2;

/**
 * @brief Runs a command body, translating exceptions into rendered errors and exit codes.
 *
 * SnipException -> renderError(what), exit 1. Any other std::exception -> exit 2.
 */
template <typename Body>
int runGuarded(view::ISnippetView& view, Body&& body) {
    try {
        return std::forward<Body>(body)();
    } catch (const common::SnipException& ex) {
        view.renderError(ex.what());
        return kExitError;
    } catch (const std::exception& ex) {
        view.renderError(std::string("Unexpected error: ") + ex.what());
        return kExitUnexpected;
    }
}

inline constexpr const char* kMultiWordHint =
    "Unexpected extra argument(s). For multi-word snippet titles, please enclose in quotes "
    "(e.g., 'Binary Search').";

/// A resolved reference to exactly one snippet, either by numeric ID or by title.
struct SnippetSelector {
    std::optional<uint64_t> id;
    std::string title;

    /// Human-readable form used in messages: "ID #3" or "'Dijkstra'".
    [[nodiscard]] std::string describe() const {
        return id ? "ID #" + std::to_string(*id) : "'" + title + "'";
    }
};

/// Either a selector or a user-facing error message.
struct SelectorResult {
    std::optional<SnippetSelector> selector;
    std::string error;
};

/**
 * @brief Resolves --id / --title / a single positional argument into a SnippetSelector.
 *
 * Shared by `get` and `delete`. A positional that looks numeric is treated as an ID,
 * otherwise as a title. @p missingMessage is returned when no selector was supplied.
 */
inline SelectorResult resolveSelector(const ParsedArgs& args, const std::string& missingMessage) {
    const bool hasId = args.hasOption("id");
    const bool hasTitle = args.hasOption("title");
    const auto& positionals = args.getPositionals();

    if (hasId && hasTitle) {
        return {std::nullopt,
                "Conflicting selector options: specify either --id or --title, not both."};
    }
    if ((hasId || hasTitle) && !positionals.empty()) {
        return {std::nullopt, std::string("Unexpected positional argument(s) when option '") +
                                  (hasId ? "--id" : "--title") + "' is provided."};
    }
    if (positionals.size() > 1) {
        return {std::nullopt, kMultiWordHint};
    }

    auto byId = [](const std::string& text) -> SelectorResult {
        if (auto id = common::parseStrictId(text)) {
            return {SnippetSelector{id, {}}, {}};
        }
        return {std::nullopt,
                "Invalid snippet ID format: '" + text + "'. Expected a positive integer."};
    };

    if (hasId) {
        return byId(args.getOption("id"));
    }
    if (hasTitle) {
        return {SnippetSelector{std::nullopt, args.getOption("title")}, {}};
    }
    if (!positionals.empty()) {
        const std::string& query = positionals.front();
        if (common::looksNumeric(query)) {
            return byId(query);
        }
        return {SnippetSelector{std::nullopt, query}, {}};
    }
    return {std::nullopt, missingMessage};
}

}  // namespace snipcli::controller::commands::detail
