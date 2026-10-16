#include "snipcli/controller/commands/get_command.hpp"
#include "snipcli/common/exceptions.hpp"
#include <iostream>

namespace snipcli::controller::commands {

GetCommand::GetCommand(ParsedArgs args,
                       std::shared_ptr<model::ISnippetReader> reader,
                       std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)),
      m_reader(std::move(reader)),
      m_view(std::move(view)) {}

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
            uint64_t id = std::stoull(m_args.getOption("id"));
            snippet = m_reader->getById(id);
        } else if (m_args.hasOption("title")) {
            snippet = m_reader->getByTitle(m_args.getOption("title"));
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            try {
                uint64_t id = std::stoull(query);
                snippet = m_reader->getById(id);
            } catch (...) {
                snippet = m_reader->getByTitle(query);
            }
        }

        if (!snippet.has_value()) {
            m_view->renderError("Snippet not found matching requested criteria.");
            return 1;
        }

        if (m_args.hasFlag("--code-only") || m_args.hasFlag("-c")) {
            std::cout << snippet->getCode();
            if (!snippet->getCode().empty() && snippet->getCode().back() != '\n') {
                std::cout << "\n";
            }
        } else {
            m_view->renderSnippet(*snippet);
        }

        return 0;
    } catch (const std::exception& ex) {
        m_view->renderError(ex.what());
        return 1;
    }
}

} // namespace snipcli::controller::commands
