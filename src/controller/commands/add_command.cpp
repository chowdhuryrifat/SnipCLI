#include "snipcli/controller/commands/add_command.hpp"

#include <fstream>
#include <sstream>

#include "command_support.hpp"
#include "snipcli/model/snippet_validator.hpp"

namespace snipcli::controller::commands {

namespace {

/// Splits a comma-separated tag list, trimming whitespace and dropping empty entries.
std::vector<std::string> splitTags(std::string_view raw) {
    std::vector<std::string> tags;
    while (!raw.empty()) {
        const size_t comma = raw.find(',');
        const std::string_view item = common::trim(raw.substr(0, comma));
        if (!item.empty()) {
            tags.emplace_back(item);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        raw.remove_prefix(comma + 1);
    }
    return tags;
}

std::string readSourceFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw common::StorageException("Unable to read source code file: " + filePath);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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

std::string AddCommand::getHelp() const {
    return "SnipCLI - add command\n"
           "Usage: snip add --title <title> --lang <lang> [--code <code> | --file <file>] "
           "[--tags <tags>]\n\n"
           "Create and persist a new code snippet in the repository.\n\n"
           "Options:\n"
           "  -t, --title <title>          Unique snippet title (alphanumeric, spaces, hyphens, "
           "underscores)\n"
           "  -l, --lang, --language <l>   Programming language (e.g. cpp, python, rust, go)\n"
           "  -c, --code <code>            Source code content as an inline string (use "
           "--code=... if leading dashes)\n"
           "  -f, --file <file>            Path to file containing source code to import\n"
           "  --tags, --tag <tags>         Comma-separated category tags (e.g. "
           "\"graph,shortest_path\")\n"
           "  --format <fmt>               Output format: 'terminal' or 'markdown' (alias 'md')\n\n"
           "Requirements:\n"
           "  • --title and --lang are required (there is no default language).\n"
           "  • Exactly one of --code or --file must be specified.\n"
           "  • Snippet titles must be unique across the repository.\n\n"
           "Examples:\n"
           "  snip add --title \"Dijkstra\" --lang cpp --file ./dijkstra.cpp --tags "
           "\"graph,algo\"\n"
           "  snip add --title \"QuickSort\" --lang python --code \"def qsort(a): return a\"\n";
}

int AddCommand::execute() {
    return detail::runGuarded(*m_view, [this] {
        const auto& positionals = m_args.getPositionals();
        if ((m_args.hasOption("title") && !positionals.empty()) || positionals.size() > 1) {
            m_view->renderError(detail::kMultiWordHint);
            return detail::kExitError;
        }

        // The language drives search/list filtering and Markdown fences, so it is never guessed.
        if (!m_args.hasOption("lang")) {
            m_view->renderError(
                "Missing required option '--lang <language>'. Specify the snippet's language, "
                "e.g. --lang cpp or -l python.");
            return detail::kExitError;
        }

        std::string title = m_args.getOption("title");
        if (title.empty() && !positionals.empty()) {
            title = positionals.front();
        }

        std::string code = m_args.getOption("code");
        if (code.empty() && m_args.hasOption("file")) {
            code = readSourceFile(m_args.getOption("file"));
        }

        model::Snippet snippet(0, std::move(title), m_args.getOption("lang"), std::move(code),
                               splitTags(m_args.getOption("tag")), "", "");
        model::SnippetValidator::validate(snippet);

        if (m_reader->exists(snippet.getTitle())) {
            throw common::ValidationException("A snippet with title '" + snippet.getTitle() +
                                              "' already exists.");
        }

        const uint64_t newId = m_writer->save(snippet);
        m_view->renderSuccess("Snippet '" + snippet.getTitle() +
                              "' created successfully with ID #" + std::to_string(newId));
        return 0;
    });
}

}  // namespace snipcli::controller::commands
