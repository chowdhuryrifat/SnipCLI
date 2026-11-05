#include "snipcli/controller/commands/stats_command.hpp"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "command_support.hpp"

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

std::string StatsCommand::getHelp() const {
    return "SnipCLI - stats command\n"
           "Usage: snip stats [options]\n\n"
           "Display code metrics and repository telemetry across all snippets.\n\n"
           "Options:\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Metrics Reported:\n"
           "  • Total stored snippet volume\n"
           "  • Total lines of code across all snippets\n"
           "  • Language distribution breakdown sorted by snippet count\n\n"
           "Examples:\n"
           "  snip stats\n"
           "  snip stats --format markdown\n";
}

int StatsCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        if (!m_args.getPositionals().empty()) {
            m_view->renderError(
                "Unexpected extra argument(s). Command 'stats' does not accept positional "
                "arguments.");
            return detail::kExitError;
        }

        const auto snippets = m_reader->getAll();
        size_t totalLines = 0;
        std::map<std::string, size_t> counts;
        for (const auto& s : snippets) {
            totalLines += s.getLineCount();
            ++counts[s.getLanguage()];
        }

        // Sort by descending snippet count, then alphabetically by language.
        std::vector<std::pair<std::string, size_t>> breakdown(counts.begin(), counts.end());
        std::sort(breakdown.begin(), breakdown.end(), [](const auto& a, const auto& b) {
            return a.second != b.second ? a.second > b.second : a.first < b.first;
        });

        m_view->renderStats(snippets.size(), totalLines, breakdown);
        return 0;
    });
}

}  // namespace snipcli::controller::commands
