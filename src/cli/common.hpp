#ifndef BANJO_CLI_COMMON_H
#define BANJO_CLI_COMMON_H

#include "process.hpp"

#include <string_view>

namespace banjo {
namespace cli {

extern bool quiet;
extern bool verbose;

void print_step(std::string_view message);
void print_command(std::string_view name, const Command &command);
void print_clear_line();
void print_empty_line();
void print_error(std::string_view message);
void exit_error();
void error(std::string_view message);

} // namespace cli
} // namespace banjo

#endif
