#include "snipcli/controller/commands/list_command.hpp"
#include "snipcli/common/exceptions.hpp"
#include <algorithm>

namespace snipcli::controller::commands {

ListCommand::ListCommand(ParsedArgs args,
                         std::shared_ptr<model::ISnippetReader> reader,
                         std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)),
      m_reader(std::move(reader)),
      m_view(std::move(view)) {}

std::string ListCommand::getName() const {
    return "list";
}

std::string ListCommand::getDescription() const {
    return "List all stored code snippets in the repository";
}

int ListCommand::execute() {
    try {
        auto snippets = m_reader->getAll();

        if (m_args.hasOption("tag")) {
            std::string tag = m_args.getOption("tag");
            std::vector<model::Snippet> filtered;
            for (const auto& s : snippets) {
                if (s.hasTag(tag)) filtered.push_back(s);
            }
            snippets = std::move(filtered);
        }

        if (m_args.hasOption("lang")) {
            std::string lang = m_args.getOption("lang");
            std::vector<model::Snippet> filtered;
            for (const auto& s : snippets) {
                if (s.getLanguage() == lang) filtered.push_back(s);
            }
            snippets = std::move(filtered);
        }

        m_view->renderSnippetList(snippets);
        return 0;
    } catch (const std::exception& ex) {
        m_view->renderError(ex.what());
        return 1;
    }
}

} // namespace snipcli::controller::commands
