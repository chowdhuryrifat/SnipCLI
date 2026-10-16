#include "snipcli/controller/commands/delete_command.hpp"

#include "snipcli/common/exceptions.hpp"

namespace snipcli::controller::commands {

DeleteCommand::DeleteCommand(ParsedArgs args, std::shared_ptr<model::ISnippetWriter> writer,
                             std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)), m_writer(std::move(writer)), m_view(std::move(view)) {}

std::string DeleteCommand::getName() const {
    return "delete";
}

std::string DeleteCommand::getDescription() const {
    return "Delete a snippet by ID or title";
}

int DeleteCommand::execute() {
    try {
        bool deleted = false;
        std::string identifier;

        if (m_args.hasOption("id")) {
            uint64_t id = std::stoull(m_args.getOption("id"));
            identifier = "ID #" + std::to_string(id);
            deleted = m_writer->removeById(id);
        } else if (m_args.hasOption("title")) {
            std::string title = m_args.getOption("title");
            identifier = "'" + title + "'";
            deleted = m_writer->removeByTitle(title);
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            try {
                uint64_t id = std::stoull(query);
                identifier = "ID #" + std::to_string(id);
                deleted = m_writer->removeById(id);
            } catch (...) {
                identifier = "'" + query + "'";
                deleted = m_writer->removeByTitle(query);
            }
        } else {
            m_view->renderError("Must specify snippet --id or --title to delete.");
            return 1;
        }

        if (deleted) {
            m_view->renderSuccess("Snippet " + identifier + " deleted successfully.");
            return 0;
        } else {
            m_view->renderError("Snippet " + identifier + " not found.");
            return 1;
        }
    } catch (const std::exception& ex) {
        m_view->renderError(ex.what());
        return 1;
    }
}

}  // namespace snipcli::controller::commands
