#ifndef BANJO_CLI_PROCESS_H
#define BANJO_CLI_PROCESS_H

#include "banjo/utils/platform.hpp"

#include <optional>
#include <string>
#include <vector>

#if OS_WINDOWS
typedef void *HANDLE;
#endif

namespace banjo {
namespace cli {

struct Command {
    enum class Stream {
        INHERIT,
        PIPE,
    };

    std::string executable;
    std::vector<std::string> args;
    Stream stdin_stream = Stream::PIPE;
    Stream stdout_stream = Stream::PIPE;
    Stream stderr_stream = Stream::PIPE;
};

struct ProcessResult {
    int exit_code;
    std::string stdout_buffer;
    std::string stderr_buffer;
};

class Process {

private:
#if OS_WINDOWS
    HANDLE process;
    HANDLE thread;
    HANDLE stdin_write_handle;
    HANDLE stdout_read_handle;
    HANDLE stderr_read_handle;
#elif OS_LINUX || OS_MACOS
    int pid;
    int stdin_write_fd;
    int stdout_read_fd;
    int stderr_read_fd;
#endif

public:
    static std::optional<Process> spawn(const Command &command);
    ProcessResult wait();

private:
#if OS_WINDOWS
    std::string read_all(HANDLE file);
#elif OS_LINUX || OS_MACOS
    std::string read_all(int fd);
#endif
};

} // namespace cli
} // namespace banjo

#endif
