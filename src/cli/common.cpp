#include "common.hpp"

#include <cstdlib>
#include <iostream>

namespace banjo {
namespace cli {

bool quiet = false;
bool verbose = false;

void print_step(std::string_view message) {
    if (verbose) {
        std::cout << message << "\n" << std::flush;
        return;
    }

    if (quiet) {
        return;
    }

    std::cout << "\x1b[2m\x1b[2K\r" << message << "\x1b[0m\r" << std::flush;
}

void print_command(std::string_view name, const Command &command) {
    if (!verbose) {
        return;
    }

    std::cout << "[" << name << "] " << command.executable;

    for (const std::string &arg : command.args) {
        std::cout << " " << arg;
    }

    std::cout << "\n";
}

void print_clear_line() {
    if (quiet || verbose) {
        return;
    }

    std::cout << "\x1b[2K\r" << std::flush;
}

void error(std::string_view message) {
    std::cout << "\x1b[31;22merror\x1b[39;22m: " << message << "\n";
    std::exit(1);
}

} // namespace cli
} // namespace banjo
