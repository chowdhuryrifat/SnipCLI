#include <iostream>
#include <memory>

#include "snipcli/common/exceptions.hpp"
#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/command_dispatcher.hpp"
#include "snipcli/controller/commands/add_command.hpp"
#include "snipcli/controller/commands/delete_command.hpp"
#include "snipcli/controller/commands/get_command.hpp"
#include "snipcli/controller/commands/list_command.hpp"
#include "snipcli/controller/commands/search_command.hpp"
#include "snipcli/controller/commands/stats_command.hpp"
#include "snipcli/model/json_snippet_repository.hpp"
#include "snipcli/view/view_factory.hpp"

int main(int argc, char* argv[]) {
    using namespace snipcli;

    try {
        const controller::ParsedArgs args = controller::CliParser::parse(argc, argv);

        if (args.hasFlag("--version") || args.hasFlag("-v")) {
            controller::CommandDispatcher::printVersion(std::cout);
            return 0;
        }

        const bool helpRequested =
            args.hasFlag("--help") || args.hasFlag("-h") || args.verb == "help";

        // Storage (Dependency Injection). Help must work even when no storage location can be
        // resolved (e.g. `env -i snip --help`), so storage errors are only fatal otherwise.
        std::shared_ptr<model::JsonSnippetRepository> repository;
        try {
            repository = std::make_shared<model::JsonSnippetRepository>();
        } catch (const common::StorageException&) {
            if (!helpRequested && !args.verb.empty()) {
                throw;
            }
        }

        const std::shared_ptr<view::ISnippetView> view =
            view::ViewFactory::fromString(args.getOption("format", "terminal"));

        // Command registration (Open/Closed Principle: new verbs plug in here only)
        using namespace controller::commands;
        controller::CommandDispatcher dispatcher;
        dispatcher.registerCommand(
            std::make_unique<AddCommand>(args, repository, repository, view));
        dispatcher.registerCommand(std::make_unique<ListCommand>(args, repository, view));
        dispatcher.registerCommand(std::make_unique<GetCommand>(args, repository, view));
        dispatcher.registerCommand(std::make_unique<DeleteCommand>(args, repository, view));
        dispatcher.registerCommand(std::make_unique<SearchCommand>(args, repository, view));
        dispatcher.registerCommand(std::make_unique<StatsCommand>(args, repository, view));

        if (helpRequested) {
            // `snip help <cmd>` targets the positional; `snip <cmd> --help` targets the verb.
            std::string target = args.verb;
            if (args.verb == "help") {
                target = args.positionals.empty() ? std::string() : args.positionals.front();
            }
            dispatcher.printHelp(std::cout, target);
            return 0;
        }

        if (args.verb.empty()) {
            if (argc <= 1) {
                dispatcher.printHelp();
                return 0;
            }
            std::cerr
                << "[✗] Error: No command specified. Use --help to view available commands.\n";
            return 1;
        }

        return dispatcher.dispatch(args.verb);
    } catch (const common::SnipException& ex) {
        std::cerr << "[✗] Error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Error: " << ex.what() << "\n";
        return 1;
    }
}
