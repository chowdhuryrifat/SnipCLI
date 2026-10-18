#include "snipcli/controller/commands/list_command.hpp"

#include <vector>

#include "snipcli/common/exceptions.hpp"
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

int ListCommand::execute() {
    try {
        auto snippets = m_reader->getAll();

        if (m_args.hasOption("tag") || m_args.hasOption("tags")) {
            std::string tag = m_args.getOption("tag", m_args.getOption("tags"));
            model::strategy::TagSearchStrategy tagStrategy;
            snippets = tagStrategy.filter(snippets, tag);
        }

        if (m_args.hasOption("lang") || m_args.hasOption("language")) {
            std::string lang = m_args.getOption("lang", m_args.getOption("language"));
            model::strategy::LanguageSearchStrategy langStrategy;
            snippets = langStrategy.filter(snippets, lang);
        }

        m_view->renderSnippetList(snippets);
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
