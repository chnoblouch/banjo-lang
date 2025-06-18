#include "common.hpp"

#include "banjo/utils/utils.hpp"

#include <iostream>

namespace banjo {
namespace cli {

bool quiet = false;
bool verbose = false;
bool single_line_output = true;

void print_step(std::string_view message) {
    if (quiet) {
        return;
    }

    if (!single_line_output) {
        std::cout << message << "\n" << std::flush;
        return;
    }

    std::cout << "\x1b[2m\x1b[2K\r" << message << "\x1b[0m\r" << std::flush;
}

void print_command(std::string_view name, const Command &command) {
    if (quiet || !verbose) {
        return;
    }

    std::cout << "[" << name << "] " << command.executable;

    for (const std::string &arg : command.args) {
        std::cout << " " << arg;
    }

    std::cout << "\n";
}

void print_clear_line() {
    if (quiet || !single_line_output) {
        return;
    }

    std::cout << "\x1b[2K\r" << std::flush;
}

void print_empty_line() {
    print_clear_line();
    std::cout << "\n";
}

void print_error(std::string_view message) {
    std::cerr << "\x1b[31;22merror\x1b[39;22m: " << message << "\n";
}

void exit_error() {
    std::exit(1);
}

void error(std::string_view message) {
    print_error(message);
    exit_error();
}

std::optional<std::filesystem::path> find_tool(std::string_view name) {
    std::string file_name(name);

#if OS_WINDOWS
    file_name += ".exe";
#endif

    std::optional<std::string_view> path_env = Utils::get_env("PATH");
    if (!path_env) {
        return {};
    }

#if OS_WINDOWS
    char delimiter = ';';
#else
    char delimiter = ':';
#endif

    std::vector<std::string_view> search_paths = Utils::split_string(*path_env, delimiter);

    for (std::string_view search_path : search_paths) {
        std::filesystem::path tool_path = std::filesystem::absolute(search_path) / file_name;

        if (std::filesystem::is_regular_file(tool_path)) {
            return tool_path;
        }
    }

    return {};
}

std::string get_tool_output(const std::filesystem::path &tool_path, std::vector<std::string> args) {
    std::string file_name = tool_path.filename().string();

    Command command{
        .executable = tool_path.string(),
        .args = std::move(args),
    };

    std::optional<Process> process = Process::spawn(command);
    if (!process) {
        error("failed to run tool `" + file_name + "`");
    }

    ProcessResult result = process->wait();
    if (result.exit_code != 0) {
        print_error("tool `" + file_name + "` returned with exit code " + std::to_string(result.exit_code));
        print_empty_line();
        std::cerr << result.stderr_buffer;
        exit_error();
    }

    std::string &stdout_ = result.stdout_buffer;

    std::string output;
    output.reserve(stdout_.size());

    for (unsigned i = 0; i < stdout_.size(); i++) {
        if (i <= stdout_.size() - 2 && stdout_[i] == '\r' && stdout_[i + 1] == '\n') {
            output.push_back('\n');
        } else {
            output.push_back(stdout_[i]);
        }
    }

    if (output.ends_with('\n')) {
        output = output.substr(0, output.size() - 1);
    }

    return output;
}

} // namespace cli
} // namespace banjo
