#include "snipcli/controller/commands/delete_command.hpp"

#include "command_support.hpp"

namespace snipcli::controller::commands {

DeleteCommand::DeleteCommand(ParsedArgs args, std::shared_ptr<model::ISnippetWriter> writer,
                             std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_writer(std::move(writer)), m_view(std::move(view)) {}

std::string DeleteCommand::getName() const {
    return "delete";
}

std::string DeleteCommand::getDescription() const {
    return "Delete a snippet by ID or title";
}

std::string DeleteCommand::getHelp() const {
    return "SnipCLI - delete command\n"
           "Usage: snip delete [--id <id> | --title <title> | <query>] [options]\n\n"
           "Delete a snippet from the repository by ID or title.\n\n"
           "Options:\n"
           "  --id <id>                    Exact numeric identifier of the snippet to delete\n"
           "  -t, --title <title>          Exact title of the snippet to delete\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Notes:\n"
           "  • Specify either --id or --title, not both.\n\n"
           "Examples:\n"
           "  snip delete --id 1\n"
           "  snip delete --title \"Dijkstra\"\n";
}

int DeleteCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        const auto resolved = detail::resolveSelector(
            m_args,
            "Must specify snippet --id <id> or --title <title> to delete, or provide an "
            "identifier as an argument.");
        if (!resolved.selector) {
            m_view->renderError(resolved.error);
            return detail::kExitError;
        }

        const auto& sel = *resolved.selector;
        const bool deleted =
            sel.id ? m_writer->removeById(*sel.id) : m_writer->removeByTitle(sel.title);
        if (!deleted) {
            m_view->renderError("Snippet " + sel.describe() + " not found.");
            return detail::kExitError;
        }

        m_view->renderSuccess("Snippet " + sel.describe() +
                              " deleted successfully. Remaining snippet IDs have been "
                              "renumbered sequentially (run 'snip list' to see them).");
        return 0;
    });
}

}  // namespace snipcli::controller::commands
