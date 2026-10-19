#include <cstdlib>
#include <filesystem>
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
        controller::ParsedArgs args = controller::CliParser::parse(argc, argv);

        // Global flags handling
        if (args.hasFlag("--version") || args.hasFlag("-v")) {
            std::cout << "SnipCLI version 1.0.0 (Modern C++17 MVC Architecture)\n";
            return 0;
        }

        // Storage and View Initialization (Dependency Injection)
        auto repository = std::make_shared<model::JsonSnippetRepository>();

        std::string formatOpt = args.getOption("format", "terminal");
        std::shared_ptr<view::ISnippetView> view = view::ViewFactory::fromString(formatOpt);

        // Command Dispatcher Setup (Open/Closed Principle)
        controller::CommandDispatcher dispatcher;
        dispatcher.registerCommand(
            std::make_unique<controller::commands::AddCommand>(args, repository, repository, view));
        dispatcher.registerCommand(
            std::make_unique<controller::commands::ListCommand>(args, repository, view));
        dispatcher.registerCommand(
            std::make_unique<controller::commands::GetCommand>(args, repository, view));
        dispatcher.registerCommand(
            std::make_unique<controller::commands::DeleteCommand>(args, repository, view));
        dispatcher.registerCommand(
            std::make_unique<controller::commands::SearchCommand>(args, repository, view));
        dispatcher.registerCommand(
            std::make_unique<controller::commands::StatsCommand>(args, repository, view));

        if (args.verb.empty() || args.hasFlag("--help") || args.hasFlag("-h")) {
            dispatcher.printHelp();
            return 0;
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
