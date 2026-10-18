#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command for inspecting or piping specific snippets.
 *
 * Supports querying snippets by unique numerical ID or unique title, supporting both full
 * formatted presentation (with line numbers and metadata headers) and raw code extraction
 * mode (`--code-only` / `-c`) for direct terminal pipelining into clipboard tools or compilers.
 */
class GetCommand : public ICommand {
public:
    /**
     * @brief Constructs a GetCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param reader Read-only data access interface to query snippets.
     * @param view Presentation sink for rendering formatted snippets, raw code, or errors.
     */
    GetCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
               std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Executes snippet lookup and display or code piping.
     * @return int Standard process exit code (0 for success, non-zero for query/storage error).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "get".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the snippet inspection workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
