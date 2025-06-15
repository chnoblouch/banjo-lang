#ifndef BANJO_CLI_COMMON_H
#define BANJO_CLI_COMMON_H

#include "process.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace banjo {
namespace cli {

extern bool quiet;
extern bool verbose;
extern bool single_line_output;

void print_step(std::string_view message);
void print_command(std::string_view name, const Command &command);
void print_clear_line();
void print_empty_line();
void print_error(std::string_view message);
void exit_error();
[[noreturn]] void error(std::string_view message);

std::optional<std::filesystem::path> find_tool(std::string_view name);
std::string get_tool_output(const std::filesystem::path &tool_path, std::vector<std::string> args);

} // namespace cli
} // namespace banjo

#endif
