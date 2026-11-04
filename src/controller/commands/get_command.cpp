#include "snipcli/controller/commands/get_command.hpp"

#include "command_support.hpp"

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

std::string GetCommand::getHelp() const {
    return "SnipCLI - get command\n"
           "Usage: snip get [--id <id> | --title <title> | <query>] [options]\n\n"
           "Retrieve and display a code snippet by ID or title.\n\n"
           "Options:\n"
           "  --id <id>                    Exact numeric identifier of the snippet\n"
           "  -t, --title <title>          Exact title of the snippet\n"
           "  -c, --code-only              Output raw source code byte-for-byte (pipe-friendly, "
           "no headers)\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Notes:\n"
           "  • Specify either --id or --title, not both.\n"
           "  • With --code-only, raw source code is emitted without metadata, perfect for "
           "clipboard pipes.\n\n"
           "Examples:\n"
           "  snip get --id 1\n"
           "  snip get --title \"Dijkstra\"\n"
           "  snip get --title \"Dijkstra\" --code-only | pbcopy\n"
           "  snip get --id 1 --code-only > solution.cpp\n";
}

int GetCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        const auto resolved = detail::resolveSelector(
            m_args,
            "Snippet lookup criteria missing. Specify --id <id>, --title <title>, or provide an "
            "identifier as an argument.");
        if (!resolved.selector) {
            m_view->renderError(resolved.error);
            return detail::kExitError;
        }

        const auto& sel = *resolved.selector;
        const auto snippet = sel.id ? m_reader->getById(*sel.id) : m_reader->getByTitle(sel.title);
        if (!snippet) {
            m_view->renderError("Snippet not found matching requested criteria.");
            return detail::kExitError;
        }

        if (m_args.hasFlag("--code-only") || m_args.hasFlag("-c")) {
            m_view->renderRaw(snippet->getCode());
        } else {
            m_view->renderSnippet(*snippet);
        }
        return 0;
    });
}

}  // namespace snipcli::controller::commands
