#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command for calculating and displaying repository telemetry.
 *
 * Traverses repository records via ISnippetReader to aggregate repository-wide metrics:
 * total snippet count, total lines of code, and snippet distribution across programming languages.
 * Formats results through the injected ISnippetView sink.
 */
class StatsCommand : public ICommand {
public:
    /**
     * @brief Constructs a StatsCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param reader Read-only data access interface to query all snippet records.
     * @param view Presentation sink for rendering aggregate telemetry metrics or error notices.
     */
    StatsCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                 std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Computes snippet metrics and delegates presentation to the View.
     * @return int Standard process exit code (0 for success, non-zero for query/system error).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "stats".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the statistics telemetry workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
