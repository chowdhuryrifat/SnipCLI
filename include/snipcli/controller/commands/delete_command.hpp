#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command for deleting snippets from repository.
 *
 * Supports deletion targeted by numerical ID (--id <numeric>) or unique title (--title <title>).
 * Also supports identifying snippets via positional arguments. Ensures clear user-facing
 * feedback for both successful removals and non-existent record requests.
 */
class DeleteCommand : public ICommand {
public:
    /**
     * @brief Constructs a DeleteCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param writer Data sink interface for mutating repository state and removing records.
     * @param view Presentation sink for rendering success notices or error messages.
     */
    DeleteCommand(ParsedArgs args, std::shared_ptr<model::ISnippetWriter> writer,
                  std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Executes snippet deletion against repository storage.
     * @return int Standard process exit code (0 for success, non-zero for error/not found).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "delete".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the deletion workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetWriter> m_writer;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
