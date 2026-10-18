#include "snipcli/controller/commands/search_command.hpp"

#include "snipcli/common/exceptions.hpp"
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

int SearchCommand::execute() {
    try {
        std::unique_ptr<model::strategy::ISearchStrategy> strategy;
        std::string criteria;

        if (m_args.hasOption("tag") || m_args.hasOption("tags")) {
            strategy = std::make_unique<model::strategy::TagSearchStrategy>();
            criteria = m_args.getOption("tag", m_args.getOption("tags"));
        } else if (m_args.hasOption("lang") || m_args.hasOption("language")) {
            strategy = std::make_unique<model::strategy::LanguageSearchStrategy>();
            criteria = m_args.getOption("lang", m_args.getOption("language"));
        } else if (m_args.hasOption("title")) {
            strategy = std::make_unique<model::strategy::TitleSearchStrategy>();
            criteria = m_args.getOption("title");
        } else if (!m_args.positionals.empty()) {
            strategy = std::make_unique<model::strategy::TitleSearchStrategy>();
            criteria = m_args.positionals[0];
        } else {
            m_view->renderError(
                "Search criteria missing. Provide a search query, --tag <tag>, or --lang "
                "<language>.");
            return 1;
        }

        auto allSnippets = m_reader->getAll();
        auto matches = strategy->filter(allSnippets, criteria);

        m_view->renderSnippetList(matches);
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
