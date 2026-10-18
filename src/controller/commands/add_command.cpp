#include "snipcli/controller/commands/add_command.hpp"

#include <fstream>
#include <sstream>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/model/snippet_validator.hpp"

namespace snipcli::controller::commands {

namespace {

std::vector<std::string> splitTags(const std::string& raw) {
    std::vector<std::string> tags;
    if (raw.empty()) {
        return tags;
    }
    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t start = item.find_first_not_of(" \t\r\n");
        size_t end = item.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            tags.push_back(item.substr(start, end - start + 1));
        }
    }
    return tags;
}

}  // namespace

AddCommand::AddCommand(ParsedArgs args, std::shared_ptr<model::ISnippetWriter> writer,
                       std::shared_ptr<model::ISnippetReader> reader,
                       std::shared_ptr<view::ISnippetView> view)
    : m_args(std::move(args)),
      m_writer(std::move(writer)),
      m_reader(std::move(reader)),
      m_view(std::move(view)) {}

std::string AddCommand::getName() const {
    return "add";
}

std::string AddCommand::getDescription() const {
    return "Create and store a new code snippet";
}

int AddCommand::execute() {
    try {
        std::string title = m_args.getOption("title");
        if (title.empty() && !m_args.positionals.empty()) {
            title = m_args.positionals[0];
        }

        std::string lang = m_args.getOption("lang", m_args.getOption("language", "cpp"));
        std::string tagsStr = m_args.getOption("tags", m_args.getOption("tag"));
        std::vector<std::string> tags = splitTags(tagsStr);

        std::string code = m_args.getOption("code");
        if (code.empty() && m_args.hasOption("file")) {
            std::string filePath = m_args.getOption("file");
            std::ifstream file(filePath);
            if (!file.is_open()) {
                throw common::StorageException("Unable to read source code file: " + filePath);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            code = buffer.str();
        }

        model::Snippet snippet(0, title, lang, code, tags, "", "");
        model::SnippetValidator::validate(snippet);

        if (m_reader->exists(snippet.getTitle())) {
            throw common::ValidationException("A snippet with title '" + snippet.getTitle() +
                                              "' already exists.");
        }

        uint64_t newId = m_writer->save(snippet);
        m_view->renderSuccess("Snippet '" + snippet.getTitle() +
                              "' created successfully with ID #" + std::to_string(newId));
        return 0;
    } catch (const common::SnipException& ex) {
        m_view->renderError(ex.what());
        return 1;
    } catch (const std::exception& ex) {
        m_view->renderError(std::string("Unexpected error: ") + ex.what());
        return 2;
    }
}

}  // namespace snipcli::controller::commands
