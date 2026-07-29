#include "argument_parser.hpp"

#include "banjo/utils/macros.hpp"
#include "common.hpp"

#include <iostream>

namespace banjo::cli {

ArgumentParser::Result ArgumentParser::parse() {
    record_parents(commands);

    Result result{
        .global_options{},
        .command = nullptr,
        .command_options{},
        .command_positionals{},
    };

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg.starts_with('-')) {
            result.global_options.push_back(parse_option(options));
        } else {
            break;
        }
    }

    if (contains_help_option(result.global_options)) {
        return result;
    }

    result.command = parse_command(nullptr);
    if (!result.command) {
        return result;
    }

    while (!result.command->subcommands.empty()) {
        while (arg_index < argc) {
            std::string_view arg = argv[arg_index];

            if (arg.starts_with('-')) {
                result.command_options.push_back(parse_option(result.command->options));
            } else {
                break;
            }
        }

        if (contains_help_option(result.command_options)) {
            return result;
        }

        if (arg_index >= argc) {
            break;
        }

        const Command *subcommand = parse_command(result.command);
        if (!subcommand) {
            break;
        }

        result.command = subcommand;
    }

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg.starts_with('-')) {
            result.command_options.push_back(parse_option(result.command->options));
        } else {
            result.command_positionals.push_back(std::string{arg});
            arg_index += 1;
        }
    }

    if (contains_help_option(result.command_options)) {
        return result;
    }

    if (!result.command->subcommands.empty()) {
        error("missing subcommand");
    } else if (result.command_positionals.size() < result.command->positionals.size()) {
        std::string_view name = result.command->positionals[result.command_positionals.size()]->name;
        error("missing positional argument '" + std::string{name} + "'");
    } else if (result.command_positionals.size() > result.command->positionals.size()) {
        const std::string &value = result.command_positionals[result.command->positionals.size()];
        error("extra positional argument '" + value + "'");
    }

    return result;
}

void ArgumentParser::record_parents(const std::vector<const Command *> &commands) {
    for (const Command *command : commands) {
        for (const Command *sub_command : command->subcommands) {
            command_parents.insert(sub_command, command);
        }

        record_parents(command->subcommands);
    }
}

ArgumentParser::OptionValue ArgumentParser::parse_option(const std::vector<const Option *> &options) {
    std::string_view arg = argv[arg_index];

    if (arg == "-" || arg == "--") {
        error("empty option");
    }

    const Option *option = nullptr;
    bool letter = false;

    if (arg.starts_with("--")) {
        option = find_option(arg.substr(2), options);
    } else if (arg.starts_with("-")) {
        option = find_option(arg[1], options);
        letter = true;
    } else {
        ASSERT_UNREACHABLE;
    }

    if (!option) {
        error("unknown option '" + std::string(arg) + "'");
    }

    if (letter && arg.size() > 2) {
        if (option->type == Option::Type::FLAG) {
            error(std::string("unexpected value for flag '-") + *option->letter + "'");
        } else if (option->type == Option::Type::VALUE) {
            std::string value(arg.substr(2));
            arg_index += 1;
            return OptionValue{.option = option, .value = value};
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    arg_index += 1;

    if (option->type == Option::Type::FLAG) {
        return OptionValue{.option = option, .value = {}};
    } else if (option->type == Option::Type::VALUE) {
        if (arg_index == argc || std::string_view(argv[arg_index]).starts_with("-")) {
            error("missing value for option '--" + std::string{option->name} + "'");
        }

        std::string value = argv[arg_index];
        arg_index += 1;
        return OptionValue{.option = option, .value = value};
    } else {
        ASSERT_UNREACHABLE;
    }
}

const ArgumentParser::Command *ArgumentParser::parse_command(const Command *parent_command) {
    if (arg_index == argc) {
        return nullptr;
    }

    std::string_view arg = argv[arg_index];
    const Command *command = find_command(parent_command, arg);

    if (!command) {
        error("unknown command '" + std::string(arg) + "'");
    }

    arg_index += 1;
    return command;
}

const ArgumentParser::Option *ArgumentParser::find_option(
    std::string_view name,
    const std::vector<const Option *> &options
) {
    for (const Option *option : options) {
        if (option->name == name) {
            return option;
        }
    }

    return nullptr;
}

const ArgumentParser::Option *ArgumentParser::find_option(char letter, const std::vector<const Option *> &options) {
    for (const Option *option : options) {
        if (option->letter == letter) {
            return option;
        }
    }

    return nullptr;
}

const ArgumentParser::Command *ArgumentParser::find_command(const Command *parent_command, std::string_view name) {
    const std::vector<const Command *> &list = parent_command ? parent_command->subcommands : commands;

    for (const Command *command : list) {
        if (command->name == name) {
            return command;
        }
    }

    return nullptr;
}

void ArgumentParser::print_help() {
    std::cout << "\n";
    std::cout << "Usage: " + std::string{name} + " [command]";

    if (!options.empty()) {
        std::cout << " [options]";
    }

    std::cout << "\n";

    if (!options.empty()) {
        std::cout << "\n";
        print_options(options);
    }

    if (!commands.empty()) {
        std::cout << "\n";
        print_commands(commands);
    }

    std::cout << "\n";
}

void ArgumentParser::print_command_help(const Command &command) {
    std::cout << "\n";
    std::cout << "Description: " << command.description << "\n";
    std::cout << "\n";
    std::cout << "Usage: " << name << " " << full_name(command);

    if (!command.options.empty()) {
        std::cout << " [options]";
    }

    if (!command.subcommands.empty()) {
        std::cout << " [command]";
    } else if (!command.positionals.empty()) {
        for (const Positional *positional : command.positionals) {
            std::cout << " [" << positional->name << "]";
        }
    }

    std::cout << "\n";

    if (!command.options.empty()) {
        std::cout << "\n";
        print_options(command.options);
    }

    if (!command.subcommands.empty()) {
        std::cout << "\n";
        print_commands(command.subcommands);
    }

    std::cout << "\n";
}

void ArgumentParser::print_options(const std::vector<const Option *> &options) {
    std::cout << "Options:\n";

    unsigned longest_option_length = 0;
    bool has_letter_option = false;

    for (const Option *option : options) {
        if (option->letter) {
            has_letter_option = true;
            break;
        }
    }

    for (const Option *option : options) {
        unsigned option_length = 2 + static_cast<unsigned>(option->name.size());

        if (option->value_placeholder) {
            option_length += 1 + static_cast<unsigned>(option->value_placeholder->size());
        }

        if (has_letter_option) {
            option_length += 4;
        }

        longest_option_length = std::max(longest_option_length, option_length);
    }

    for (const Option *option : options) {
        std::cout << "  ";

        std::string name_column;

        if (has_letter_option) {
            if (option->letter) {
                name_column = std::string{"-"} + *option->letter + ", ";
            } else {
                name_column = "    ";
            }
        }

        name_column += "--" + std::string{option->name};

        if (option->value_placeholder) {
            name_column += " " + std::string{*option->value_placeholder};
        }

        std::cout << name_column;
        std::cout << std::string(longest_option_length + 3 - name_column.size(), ' ');
        std::cout << option->description;
        std::cout << "\n";
    }
}

void ArgumentParser::print_commands(const std::vector<const Command *> &commands) {
    std::cout << "Commands:\n";

    unsigned longest_command_length = 0;

    for (const Command *command : commands) {
        longest_command_length = std::max(longest_command_length, static_cast<unsigned>(command->name.size()));
    }

    for (const Command *command : commands) {
        std::cout << "  " << command->name;
        std::cout << std::string(longest_command_length + 3 - command->name.length(), ' ');
        std::cout << command->description;
        std::cout << "\n";
    }
}

std::string ArgumentParser::full_name(const Command &command) {
    std::string full_name{command.name};
    const Command *current = &command;

    while (const Command **parent = command_parents.try_find(current)) {
        current = *parent;

        full_name.insert(0, " ");
        full_name.insert(0, current->name);
    }

    return full_name;
}

bool ArgumentParser::contains_help_option(const std::vector<OptionValue> &option_values) {
    for (const OptionValue &option_value : option_values) {
        if (option_value.option->name == "help") {
            return true;
        }
    }

    return false;
}

} // namespace banjo::cli
