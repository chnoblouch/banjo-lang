#ifndef BANJO_CLI_ARGUMENT_PARSER_H
#define BANJO_CLI_ARGUMENT_PARSER_H

#include "banjo/utils/hash_map.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace banjo::cli {

class ArgumentParser {

public:
    struct Option {
        enum class Type {
            FLAG,
            VALUE,
        };

        Type type;
        std::string_view name;
        std::optional<char> letter;
        std::string_view description;
        std::optional<std::string_view> value_placeholder;

        Option(Type type, std::string_view name, char letter, std::string_view description)
          : type{type},
            name{name},
            letter{letter},
            description{description} {}

        Option(Type type, std::string_view name, std::string_view description)
          : type{type},
            name{name},
            letter{},
            description{description} {}

        Option(Type type, std::string_view name, std::string value_placeholder, std::string_view description)
          : type{type},
            name{name},
            letter{},
            description{description},
            value_placeholder{value_placeholder} {}

        Option(
            Type type,
            std::string_view name,
            char letter,
            std::string_view value_placeholder,
            std::string_view description
        )
          : type{type},
            name{name},
            letter{letter},
            description{description},
            value_placeholder{value_placeholder} {}
    };

    struct Positional {
        std::string_view name;

        Positional(std::string_view name) : name{name} {}
    };

    struct Command {
        std::string_view name;
        std::string_view description;
        std::vector<const Option *> options{};
        std::vector<const Positional *> positionals{};
        std::vector<const Command *> subcommands{};
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
    std::vector<const Option *> options;
    std::vector<const Command *> commands;
    HashMap<const Command *, const Command *> command_parents;

    Result parse();
    void print_help();
    void print_command_help(const Command &command);

private:
    void record_parents(const std::vector<const Command *> &commands);

    OptionValue parse_option(const std::vector<const Option *> &options);
    const Command *parse_command(const Command *parent_command);

    const Option *find_option(std::string_view name, const std::vector<const Option *> &options);
    const Option *find_option(char letter, const std::vector<const Option *> &options);
    const Command *find_command(const Command *parent_command, std::string_view name);

    void print_options(const std::vector<const Option *> &options);
    void print_commands(const std::vector<const Command *> &commands);
    std::string full_name(const Command &command);

    static bool contains_help_option(const std::vector<OptionValue> &option_values);
};

} // namespace banjo::cli

#endif
