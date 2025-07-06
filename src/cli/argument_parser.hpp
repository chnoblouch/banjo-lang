#ifndef BANJO_CLI_ARGUMENT_PARSER_H
#define BANJO_CLI_ARGUMENT_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo {
namespace cli {

class ArgumentParser {

public:
    struct Option {
        enum class Type {
            FLAG,
            VALUE,
        };

        Type type;
        std::string name;
        std::optional<char> letter;
        std::string description;
        std::optional<std::string> value_placeholder;

        Option(Type type, std::string name, char letter, std::string description)
          : type(type),
            name(std::move(name)),
            letter(letter),
            description(std::move(description)) {}

        Option(Type type, std::string name, std::string description)
          : type(type),
            name(std::move(name)),
            letter{},
            description(std::move(description)) {}

        Option(Type type, std::string name, std::string value_placeholder, std::string description)
          : type(type),
            name(std::move(name)),
            letter{},
            description(std::move(description)),
            value_placeholder(std::move(value_placeholder)) {}

        Option(Type type, std::string name, char letter, std::string value_placeholder, std::string description)
          : type(type),
            name(std::move(name)),
            letter(letter),
            description(std::move(description)),
            value_placeholder(std::move(value_placeholder)) {}
    };

    struct Positional {
        std::string name;

        Positional(std::string name) : name(std::move(name)) {}
    };

    struct Command {
        std::string name;
        std::string description;
        std::vector<Option> options;
        std::vector<Positional> positionals;

        Command(
            std::string name,
            std::string description,
            std::vector<Option> options,
            std::vector<Positional> positionals = {}
        )
          : name(std::move(name)),
            description(std::move(description)),
            options(std::move(options)),
            positionals(std::move(positionals)) {}
    };

    struct OptionValue {
        const Option *option;
        std::optional<std::string> value;
    };

    struct Result {
        std::vector<OptionValue> global_options;
        const Command *command;
        std::vector<OptionValue> command_options;
        std::vector<std::string> command_positionals;
    };

    int argc;
    const char **argv;
    int arg_index = 1;
    std::string name;
    std::vector<Option> options;
    std::vector<Command> commands;

    Result parse();
    void print_help();
    void print_command_help(const Command &command);

private:
    OptionValue parse_option(const std::vector<Option> &options);
    const Command *parse_command();

    const Option *find_option(std::string_view name, const std::vector<Option> &options);
    const Option *find_option(char letter, const std::vector<Option> &options);
    const Command *find_command(std::string_view name);

    void print_options(const std::vector<Option> &options);
    void print_commands(const std::vector<Command> &commands);

    static bool contains_help_option(const std::vector<OptionValue> &option_values);
};

} // namespace cli
} // namespace banjo

#endif
