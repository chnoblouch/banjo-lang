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

        arg_index += 1;
    }

    result.command = parse_command();
    arg_index += 1;

    if (!result.command) {
        return result;
    }

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg.starts_with('-')) {
            result.command_options.push_back(parse_option(options));
        } else {
            result.command_positionals.push_back(std::string(arg));
        }

        arg_index += 1;
    }

    return result;
}

ArgumentParser::OptionValue ArgumentParser::parse_option(const std::vector<Option> &options) {
    std::string_view arg = argv[arg_index];

    const Option *option = nullptr;

    if (arg.starts_with("--")) {
        option = find_option(arg.substr(2), options);
    } else if (arg.size() == 2 && arg[0] == '-') {
        option = find_option(arg[1], options);
    } else {
        ASSERT_UNREACHABLE;
    }

    if (!option) {
        error("unknown option '" + std::string(arg) + "'");
    }

    if (option->type == Option::Type::FLAG) {
        return OptionValue{.option = option, .value = {}};
    } else if (option->type == Option::Type::VALUE) {
        if (arg_index == argc || std::string_view(argv[arg_index]).starts_with("-")) {
            error("missing value for option '--" + option->name + "'");
        }

        std::string value = argv[arg_index];
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
    std::cout << "Usage: " + name + " [command] [options]\n";

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
    std::cout << "Usage: " + name + " " + command.name + " [options]";

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
        unsigned option_length = 2 + static_cast<unsigned>(option.name.size());

        if (option.value_placeholder) {
            option_length += 1 + static_cast<unsigned>(option.value_placeholder->size());
        }

        longest_option_length = std::max(longest_option_length, option_length);

        if (option.letter) {
            has_letter_option = true;
        }
    }

    for (const Option &option : options) {
        std::cout << "  ";

        if (has_letter_option) {
            if (option.letter) {
                std::cout << "-" << *option.letter << ", ";
            } else {
                std::cout << "    ";
            }
        }

        std::string name_column = "--" + option.name;

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
