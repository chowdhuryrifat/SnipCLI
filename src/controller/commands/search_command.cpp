#include "snipcli/controller/commands/search_command.hpp"

#include "command_support.hpp"
#include "snipcli/model/strategy/language_search_strategy.hpp"
#include "snipcli/model/strategy/tag_search_strategy.hpp"
#include "snipcli/model/strategy/title_search_strategy.hpp"

namespace snipcli::controller::commands {

SearchCommand::SearchCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                             std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_reader(std::move(reader)), m_view(std::move(view)) {}

std::string SearchCommand::getName() const {
    return "search";
}

std::string SearchCommand::getDescription() const {
    return "Search snippets by query substring, category tag, or language using GoF Strategy "
           "Pattern";
}

std::string SearchCommand::getHelp() const {
    return "SnipCLI - search command\n"
           "Usage: snip search [<query>] [options]\n\n"
           "Search snippets by query substring, category tag, or language using the GoF "
           "Strategy Pattern.\n\n"
           "Options:\n"
           "  -t, --title <query>          Search by case-insensitive title substring\n"
           "  --tag, --tags <tag>          Search by exact category tag match\n"
           "  -l, --lang, --language <l>   Search by exact programming language\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Notes:\n"
           "  • Positional queries (e.g. 'snip search dijkstra') perform case-insensitive "
           "title search.\n\n"
           "Examples:\n"
           "  snip search \"dijkstra\"\n"
           "  snip search --tag \"graph\"\n"
           "  snip search --lang \"python\"\n"
           "  snip search --tag \"dp\" --format markdown\n";
}

int SearchCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        const auto& positionals = m_args.getPositionals();
        const bool hasFilterOption =
            m_args.hasOption("tag") || m_args.hasOption("lang") || m_args.hasOption("title");

        if (hasFilterOption && !positionals.empty()) {
            m_view->renderError(
                "Unexpected positional argument(s) when search filter options are provided.");
            return detail::kExitError;
        }
        if (positionals.size() > 1) {
            m_view->renderError(
                "Unexpected extra argument(s). For multi-word queries, please enclose in "
                "quotes (e.g., 'Binary Search').");
            return detail::kExitError;
        }

        // Strategy selection precedence: tag > language > title option > positional title query.
        std::unique_ptr<model::strategy::ISearchStrategy> strategy;
        std::string criteria;
        if (m_args.hasOption("tag")) {
            strategy = std::make_unique<model::strategy::TagSearchStrategy>();
            criteria = m_args.getOption("tag");
        } else if (m_args.hasOption("lang")) {
            strategy = std::make_unique<model::strategy::LanguageSearchStrategy>();
            criteria = m_args.getOption("lang");
        } else if (m_args.hasOption("title")) {
            strategy = std::make_unique<model::strategy::TitleSearchStrategy>();
            criteria = m_args.getOption("title");
        } else if (!positionals.empty()) {
            strategy = std::make_unique<model::strategy::TitleSearchStrategy>();
            criteria = positionals.front();
        } else {
            m_view->renderError(
                "Search criteria missing. Provide a search query, --tag <tag>, or --lang "
                "<language>.");
            return detail::kExitError;
        }

        m_view->renderSnippetList(strategy->filter(m_reader->getAll(), criteria));
        return 0;
    });
}

}  // namespace snipcli::controller::commands
