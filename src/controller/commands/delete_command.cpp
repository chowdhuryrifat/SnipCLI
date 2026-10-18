#include "snipcli/controller/commands/delete_command.hpp"

#include <string>

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
            std::string idStr = m_args.getOption("id");
            try {
                size_t idx = 0;
                uint64_t id = std::stoull(idStr, &idx);
                if (idx != idStr.size()) {
                    m_view->renderError("Invalid snippet ID format: '" + idStr +
                                        "'. Expected an integer.");
                    return 1;
                }
                identifier = "ID #" + std::to_string(id);
                deleted = m_writer->removeById(id);
            } catch (const std::exception&) {
                m_view->renderError("Invalid snippet ID format: '" + idStr +
                                    "'. Expected an integer.");
                return 1;
            }
        } else if (m_args.hasOption("title")) {
            std::string title = m_args.getOption("title");
            identifier = "'" + title + "'";
            deleted = m_writer->removeByTitle(title);
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            try {
                size_t idx = 0;
                uint64_t id = std::stoull(query, &idx);
                if (idx == query.size()) {
                    identifier = "ID #" + std::to_string(id);
                    deleted = m_writer->removeById(id);
                } else {
                    identifier = "'" + query + "'";
                    deleted = m_writer->removeByTitle(query);
                }
            } catch (...) {
                identifier = "'" + query + "'";
                deleted = m_writer->removeByTitle(query);
            }
        } else {
            m_view->renderError(
                "Must specify snippet --id <id> or --title <title> to delete, or provide an "
                "identifier as an argument.");
            return 1;
        }

        if (deleted) {
            m_view->renderSuccess("Snippet " + identifier + " deleted successfully.");
            return 0;
        } else {
            m_view->renderError("Snippet " + identifier + " not found.");
            return 1;
        }
    } catch (const common::SnipException& ex) {
        m_view->renderError(ex.what());
        return 1;
    } catch (const std::exception& ex) {
        m_view->renderError(std::string("Unexpected error: ") + ex.what());
        return 2;
    }
}

}  // namespace snipcli::controller::commands
