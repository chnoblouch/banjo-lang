#ifndef BANJO_CLI_ARGUMENT_PARSER_H
#define BANJO_CLI_ARGUMENT_PARSER_H

#include <optional>
#include <string>
#include <vector>

namespace banjo {
namespace cli {

class ArgumentParser {

public:
    struct Option {
        std::string name;
        std::optional<char> letter;
        std::string description;
        std::optional<std::string> value_placeholder;

        Option(std::string name, char letter, std::string description)
          : name(std::move(name)),
            letter(letter),
            description(std::move(description)) {}

        Option(std::string name, std::string description)
          : name(std::move(name)),
            letter{},
            description(std::move(description)) {}

        Option(std::string name, std::string value_placeholder, std::string description)
          : name(std::move(name)),
            letter{},
            description(std::move(description)),
            value_placeholder(std::move(value_placeholder)) {}
    };

    struct Command {
        std::string name;
        std::string description;
        std::vector<Option> options;

        Command(std::string name, std::string description, std::vector<Option> options)
          : name(std::move(name)),
            description(std::move(description)),
            options(std::move(options)) {}
    };

    struct Result {};

    std::string name;
    std::vector<Option> options;
    std::vector<Command> commands;

    void print_help();
    void print_command_help(const Command &command);

private:
    void print_options(const std::vector<Option> &options);
    void print_commands(const std::vector<Command> &commands);
};

} // namespace cli
} // namespace banjo

#endif
