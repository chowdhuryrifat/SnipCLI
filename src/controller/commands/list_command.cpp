#include "snipcli/controller/commands/list_command.hpp"

#include "command_support.hpp"
#include "snipcli/model/strategy/language_search_strategy.hpp"
#include "snipcli/model/strategy/tag_search_strategy.hpp"

namespace snipcli::controller::commands {

ListCommand::ListCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                         std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_reader(std::move(reader)), m_view(std::move(view)) {}

std::string ListCommand::getName() const {
    return "list";
}

std::string ListCommand::getDescription() const {
    return "List all stored code snippets in the repository";
}

std::string ListCommand::getHelp() const {
    return "SnipCLI - list command\n"
           "Usage: snip list [options]\n\n"
           "List all stored code snippets in tabular format with optional filtering.\n\n"
           "Options:\n"
           "  -l, --lang, --language <l>   Filter snippets by programming language\n"
           "  --tag, --tags <tag>          Filter snippets by category tag\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Examples:\n"
           "  snip list\n"
           "  snip list --lang cpp\n"
           "  snip list --tag graph --format markdown\n";
}

int ListCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        if (!m_args.getPositionals().empty()) {
            m_view->renderError(
                "Unexpected extra argument(s). Command 'list' does not accept positional "
                "arguments.");
            return detail::kExitError;
        }

        auto snippets = m_reader->getAll();
        if (m_args.hasOption("tag")) {
            snippets =
                model::strategy::TagSearchStrategy{}.filter(snippets, m_args.getOption("tag"));
        }
        if (m_args.hasOption("lang")) {
            snippets = model::strategy::LanguageSearchStrategy{}.filter(snippets,
                                                                        m_args.getOption("lang"));
        }

        m_view->renderSnippetList(snippets);
        return 0;
    });
}

}  // namespace snipcli::controller::commands
