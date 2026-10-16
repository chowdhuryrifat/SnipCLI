#include "snipcli/controller/commands/stats_command.hpp"

#include <map>
#include <vector>

namespace snipcli::controller::commands {

StatsCommand::StatsCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                           std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_reader(std::move(reader)), m_view(std::move(view)) {}

std::string StatsCommand::getName() const {
    return "stats";
}

std::string StatsCommand::getDescription() const {
    return "Display code metrics and statistics across all snippets";
}

int StatsCommand::execute() {
    try {
        auto snippets = m_reader->getAll();
        size_t totalSnippets = snippets.size();
        size_t totalLines = 0;
        std::map<std::string, size_t> counts;

        for (const auto& s : snippets) {
            totalLines += s.getLineCount();
            counts[s.getLanguage()]++;
        }

        std::vector<std::pair<std::string, size_t>> breakdown(counts.begin(), counts.end());
        m_view->renderStats(totalSnippets, totalLines, breakdown);
        return 0;
    } catch (const std::exception& ex) {
        m_view->renderError(ex.what());
        return 1;
    }
}

}  // namespace snipcli::controller::commands
