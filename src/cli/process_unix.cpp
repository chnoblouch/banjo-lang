#include "process.hpp"

#include "common.hpp"

#include "banjo/utils/macros.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

namespace banjo {
namespace cli {

std::optional<Process> Process::spawn(const Command &command) {
    pid_t pid = fork();

    if (pid == -1) {
        std::string message(strerror(errno));
        error("failed to spawn process: " + message);
        return {};
    }

    if (pid == 0) {
        char **argv = new char *[command.args.size() + 2];

        std::string executable = command.executable;
        std::vector<std::string> args = command.args;

        argv[0] = executable.data();

        for (unsigned i = 0; i < args.size(); i++) {
            argv[i + 1] = args[i].data();
        }

        argv[command.args.size() + 1] = NULL;

        if (execvpe(executable.data(), argv, environ) == -1) {
            std::string message(strerror(errno));
            error("failed to spawn process: " + message);
        }

        ASSERT_UNREACHABLE;
    } else {
        return Process(pid);
    }

    return {};
}

Process::Process(int pid) : pid(pid) {}

ProcessResult Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);

    return ProcessResult{
        .exit_code = WEXITSTATUS(status),
    };
}

} // namespace cli
} // namespace banjo
