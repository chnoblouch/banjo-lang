#ifndef BANJO_CLI_PROCESS_H
#define BANJO_CLI_PROCESS_H

#include "banjo/utils/platform.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#if OS_WINDOWS
typedef void *HANDLE;
#endif

namespace banjo {
namespace cli {

struct Command {
    std::string executable;
    std::vector<std::string> args;
};

struct ProcessResult {
    int exit_code;
};

class Process {

private:
#if OS_WINDOWS
    HANDLE process;
    HANDLE thread;
#elif OS_LINUX
    int pid;
#endif

public:
    static std::optional<Process> spawn(const Command &command);

private:
#if OS_WINDOWS
    Process(HANDLE process, HANDLE thread);
#elif OS_LINUX
    Process(int pid);
#endif

public:
    ProcessResult wait();
};

} // namespace cli
} // namespace banjo

#endif
