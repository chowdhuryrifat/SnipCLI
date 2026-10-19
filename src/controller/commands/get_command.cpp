#include "snipcli/controller/commands/get_command.hpp"

#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::controller::commands {

namespace {

bool isNumeric(std::string_view s) {
    if (s.empty()) {
        return false;
    }
    size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (start >= s.size()) {
        return false;
    }
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<uint64_t> parseStrictId(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }
    uint64_t id = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), id);
    if (ec != std::errc{} || p != s.data() + s.size() || id == 0) {
        return std::nullopt;
    }
    return id;
}

}  // namespace

GetCommand::GetCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                       std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_reader(std::move(reader)), m_view(std::move(view)) {}

std::string GetCommand::getName() const {
    return "get";
}

std::string GetCommand::getDescription() const {
    return "Retrieve and display a snippet by ID or title";
}

int GetCommand::execute() {
    try {
        std::optional<model::Snippet> snippet;

        if (m_args.hasOption("id")) {
            std::string idStr = m_args.getOption("id");
            auto parsedId = parseStrictId(idStr);
            if (!parsedId.has_value()) {
                m_view->renderError("Invalid snippet ID format: '" + idStr +
                                    "'. Expected a positive integer.");
                return 1;
            }
            snippet = m_reader->getById(*parsedId);
        } else if (m_args.hasOption("title")) {
            snippet = m_reader->getByTitle(m_args.getOption("title"));
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            if (isNumeric(query)) {
                auto parsedId = parseStrictId(query);
                if (!parsedId.has_value()) {
                    m_view->renderError("Invalid snippet ID format: '" + query +
                                        "'. Expected a positive integer.");
                    return 1;
                }
                snippet = m_reader->getById(*parsedId);
            } else {
                snippet = m_reader->getByTitle(query);
            }
        } else {
            m_view->renderError(
                "Snippet lookup criteria missing. Specify --id <id>, --title <title>, or provide "
                "an identifier as an argument.");
            return 1;
        }

        if (!snippet.has_value()) {
            m_view->renderError("Snippet not found matching requested criteria.");
            return 1;
        }

        if (m_args.hasFlag("--code-only") || m_args.hasFlag("-c")) {
            m_view->renderMessage(snippet->getCode());
        } else {
            m_view->renderSnippet(*snippet);
        }

        return 0;
    } catch (const common::SnipException& ex) {
        m_view->renderError(ex.what());
        return 1;
    } catch (const std::exception& ex) {
        m_view->renderError(std::string("Unexpected error: ") + ex.what());
        return 2;
    }
}

}  // namespace snipcli::controller::commands
