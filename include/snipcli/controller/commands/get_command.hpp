#pragma once

#include "snipcli/controller/cli_parser.hpp"
#include "snipcli/controller/i_command.hpp"
#include "snipcli/model/i_snippet_repository.hpp"
#include "snipcli/view/i_snippet_view.hpp"
#include <memory>

namespace snipcli::controller::commands {

class GetCommand : public ICommand {
public:
    GetCommand(ParsedArgs args,
               std::shared_ptr<model::ISnippetReader> reader,
               std::shared_ptr<view::ISnippetView> view);

    int execute() override;
    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] std::string getDescription() const override;

private:
    ParsedArgs m_args;
    std::shared_ptr<model::ISnippetReader> m_reader;
    std::shared_ptr<view::ISnippetView> m_view;
};

} // namespace snipcli::controller::commands
