#include "snipcli/controller/commands/get_command.hpp"

#include <string>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::controller::commands {

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
            try {
                size_t idx = 0;
                uint64_t id = std::stoull(idStr, &idx);
                if (idx != idStr.size()) {
                    m_view->renderError("Invalid snippet ID format: '" + idStr +
                                        "'. Expected an integer.");
                    return 1;
                }
                snippet = m_reader->getById(id);
            } catch (const std::exception&) {
                m_view->renderError("Invalid snippet ID format: '" + idStr +
                                    "'. Expected an integer.");
                return 1;
            }
        } else if (m_args.hasOption("title")) {
            snippet = m_reader->getByTitle(m_args.getOption("title"));
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            try {
                size_t idx = 0;
                uint64_t id = std::stoull(query, &idx);
                if (idx == query.size()) {
                    snippet = m_reader->getById(id);
                } else {
                    snippet = m_reader->getByTitle(query);
                }
            } catch (...) {
                snippet = m_reader->getByTitle(query);
            }
        } else {
            m_view->renderError(
                "Snippet lookup criteria missing. Specify --id <id>, --title <title>, or provide "
                "an "
                "identifier as an argument.");
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
