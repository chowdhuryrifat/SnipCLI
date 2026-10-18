#pragma once

#include <memory>
#include <string>

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"

namespace snipcli::controller::commands {

/**
 * @brief Command Pattern (GoF): Concrete command encapsulating snippet creation and ingestion.
 *
 * Coordinates reading source code (either from inline CLI arguments or external files),
 * sanitizing category tags, executing domain validation via SnippetValidator, ensuring
 * title uniqueness across repository records, and persisting new snippets.
 */
class AddCommand : public ICommand {
public:
    /**
     * @brief Constructs an AddCommand instance with injected dependencies.
     * @param args Parsed command-line arguments and option flags.
     * @param writer Data sink for persisting new snippet domain entities.
     * @param reader Data source for uniqueness checking and existing record inspection.
     * @param view Presentation sink for user feedback, success notices, and error alerts.
     */
    AddCommand(ParsedArgs args, std::shared_ptr<model::ISnippetWriter> writer,
               std::shared_ptr<model::ISnippetReader> reader,
               std::shared_ptr<view::ISnippetView> view);

    /**
     * @brief Executes snippet creation, validation, and storage.
     * @return int Standard process exit code (0 for success, non-zero for validation/storage
     * error).
     */
    int execute() override;

    /**
     * @brief Retrieves the CLI action verb associated with this command.
     * @return std::string Command verb "add".
     */
    [[nodiscard]] std::string getName() const override;

    /**
     * @brief Retrieves a human-readable summary of this command's purpose.
     * @return std::string Brief description of the snippet creation workflow.
     */
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetWriter> m_writer;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

}  // namespace snipcli::controller::commands
