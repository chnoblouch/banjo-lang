#include "argument_parser.hpp"
#include "banjo/utils/macros.hpp"
#include "common.hpp"

#include <iostream>

namespace banjo {
namespace cli {

ArgumentParser::Result ArgumentParser::parse() {
    Result result;

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg.starts_with('-')) {
            result.global_options.push_back(parse_option(options));
        } else {
            break;
        }
    }

    result.command = parse_command();
    arg_index += 1;

    if (!result.command) {
        return result;
    }

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg.starts_with('-')) {
            result.command_options.push_back(parse_option(result.command->options));
        } else {
            result.command_positionals.push_back(std::string(arg));
            arg_index += 1;
        }
    }

    if (result.command_positionals.size() < result.command->positionals.size()) {
        const std::string &name = result.command->positionals[result.command_positionals.size()].name;
        error("missing positional argument '" + name + "'");
    }

    return result;
}

ArgumentParser::OptionValue ArgumentParser::parse_option(const std::vector<Option> &options) {
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
            error("missing value for option '--" + option->name + "'");
        }

        std::string value = argv[arg_index];
        arg_index += 1;
        return OptionValue{.option = option, .value = value};
    } else {
        ASSERT_UNREACHABLE;
    }
}

const ArgumentParser::Command *ArgumentParser::parse_command() {
    if (arg_index == argc) {
        return nullptr;
    }

    std::string_view arg = argv[arg_index];
    const Command *command = find_command(arg);

    if (!command) {
        error("unknown command '" + std::string(arg) + "'");
    }

    return command;
}

const ArgumentParser::Option *ArgumentParser::find_option(std::string_view name, const std::vector<Option> &options) {
    for (const Option &option : options) {
        if (option.name == name) {
            return &option;
        }
    }

    return nullptr;
}

const ArgumentParser::Option *ArgumentParser::find_option(char letter, const std::vector<Option> &options) {
    for (const Option &option : options) {
        if (option.letter == letter) {
            return &option;
        }
    }

    return nullptr;
}

const ArgumentParser::Command *ArgumentParser::find_command(std::string_view name) {
    for (const Command &command : commands) {
        if (command.name == name) {
            return &command;
        }
    }

    return nullptr;
}

void ArgumentParser::print_help() {
    std::cout << "\n";
    std::cout << "Usage: " + name + " [command]";

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
    std::cout << "Description: " + command.description << "\n";
    std::cout << "\n";
    std::cout << "Usage: " << name << " " << command.name;

    if (!command.options.empty()) {
        std::cout << " [options]";
    }

    if (!command.positionals.empty()) {
        for (const Positional &positional : command.positionals) {
            std::cout << " [" << positional.name << "]";
        }
    }

    std::cout << "\n";

    if (!command.options.empty()) {
        std::cout << "\n";
        print_options(command.options);
    }

    std::cout << "\n";
}

void ArgumentParser::print_options(const std::vector<Option> &options) {
    std::cout << "Options:\n";

    unsigned longest_option_length = 0;
    bool has_letter_option = false;

    for (const Option &option : options) {
        if (option.letter) {
            has_letter_option = true;
            break;
        }
    }

    for (const Option &option : options) {
        unsigned option_length = 2 + static_cast<unsigned>(option.name.size());

        if (option.value_placeholder) {
            option_length += 1 + static_cast<unsigned>(option.value_placeholder->size());
        }

        if (has_letter_option) {
            option_length += 4;
        }

        longest_option_length = std::max(longest_option_length, option_length);
    }

    for (const Option &option : options) {
        std::cout << "  ";

        std::string name_column;

        if (has_letter_option) {
            if (option.letter) {
                name_column = std::string("-") + *option.letter + ", ";
            } else {
                name_column = "    ";
            }
        }

        name_column += "--" + option.name;

        if (option.value_placeholder) {
            name_column += " " + *option.value_placeholder;
        }

        std::cout << name_column;
        std::cout << std::string(longest_option_length + 3 - name_column.size(), ' ');
        std::cout << option.description;
        std::cout << "\n";
    }
}

void ArgumentParser::print_commands(const std::vector<Command> &commands) {
    std::cout << "Commands:\n";

    unsigned longest_command_length = 0;

    for (const Command &command : commands) {
        longest_command_length = std::max(longest_command_length, static_cast<unsigned>(command.name.size()));
    }

    for (const Command &command : commands) {
        std::cout << "  " << command.name;
        std::cout << std::string(longest_command_length + 3 - command.name.length(), ' ');
        std::cout << command.description;
        std::cout << "\n";
    }
}

} // namespace cli
} // namespace banjo
