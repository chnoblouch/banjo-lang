#include "argument_parser.hpp"

#include <iostream>

namespace banjo {
namespace cli {

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
    std::cout << "Usage: " + name + " " + command.name + " [options]\n";

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
        std::cout << std::string(longest_option_length + 3 - name_column.length(), ' ');
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
