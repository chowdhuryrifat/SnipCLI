#include "snipcli/controller/commands/delete_command.hpp"

#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

#include "snipcli/common/exceptions.hpp"

namespace snipcli::controller::commands {

namespace {

bool isNumeric(std::string_view s) {
    if (s.empty()) {
        return false;
    }
    size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (start >= s.size()) {
        return false;
    }
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<uint64_t> parseStrictId(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }
    uint64_t id = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), id);
    if (ec != std::errc{} || p != s.data() + s.size() || id == 0) {
        return std::nullopt;
    }
    return id;
}

}  // namespace

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
            auto parsedId = parseStrictId(idStr);
            if (!parsedId.has_value()) {
                m_view->renderError("Invalid snippet ID format: '" + idStr +
                                    "'. Expected a positive integer.");
                return 1;
            }
            identifier = "ID #" + std::to_string(*parsedId);
            deleted = m_writer->removeById(*parsedId);
        } else if (m_args.hasOption("title")) {
            std::string title = m_args.getOption("title");
            identifier = "'" + title + "'";
            deleted = m_writer->removeByTitle(title);
        } else if (!m_args.positionals.empty()) {
            std::string query = m_args.positionals[0];
            if (isNumeric(query)) {
                auto parsedId = parseStrictId(query);
                if (!parsedId.has_value()) {
                    m_view->renderError("Invalid snippet ID format: '" + query +
                                        "'. Expected a positive integer.");
                    return 1;
                }
                identifier = "ID #" + std::to_string(*parsedId);
                deleted = m_writer->removeById(*parsedId);
            } else {
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
