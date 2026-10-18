#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command for searching snippets using GoF Strategy Pattern.
 *
 * Dispatches candidate snippets to interchangeable algorithmic strategies based on CLI arguments:
 * - `--tag` / `--tags`: TagSearchStrategy (case-insensitive exact tag match)
 * - `--lang` / `--language`: LanguageSearchStrategy (case-insensitive language match)
 * - `--title` or positional argument: TitleSearchStrategy (case-insensitive substring match)
 */
class SearchCommand : public ICommand {
public:
    /**
     * @brief Constructs a SearchCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param reader Read-only data access interface to query candidate snippets.
     * @param view Presentation sink for rendering tabular search results or error messages.
     */
    SearchCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                  std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Executes algorithmic strategy selection, filtering, and result rendering.
     * @return int Standard process exit code (0 for success, non-zero for missing criteria or
     * error).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "search".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the search workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
