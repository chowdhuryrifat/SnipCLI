#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command for querying and listing repository snippets.
 *
 * Provides structured tabular listing of snippets with optional category tag or programming
 * language filtering, delegating search filtering dynamically to GoF Strategy Pattern algorithms.
 */
class ListCommand : public ICommand {
public:
    /**
     * @brief Constructs a ListCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param reader Read-only data access interface to query snippets.
     * @param view Presentation sink for rendering tabular outputs and error notifications.
     */
    ListCommand(ParsedArgs args, std::shared_ptr<model::ISnippetReader> reader,
                std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Executes the snippet listing and filtering workflow.
     * @return int Standard process exit code (0 for success, non-zero for query/storage error).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "list".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the snippet listing workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
