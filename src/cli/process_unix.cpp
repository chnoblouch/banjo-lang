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
    Process process;

    int stdin_fd[2] = {-1, -1};
    int stdout_fd[2] = {-1, -1};
    int stderr_fd[2] = {-1, -1};

    if (command.stdin_stream == Command::Stream::PIPE && pipe(stdin_fd) == -1) {
        error("`pipe` failed for stdin");
    }

    if (command.stdout_stream == Command::Stream::PIPE && pipe(stdout_fd) == -1) {
        error("`pipe` failed for stdout");
    }

    if (command.stderr_stream == Command::Stream::PIPE && pipe(stderr_fd) == -1) {
        error("`pipe` failed for stderr");
    }

    pid_t pid = fork();

    if (pid == -1) {
        std::string message(strerror(errno));
        error("failed to spawn process: " + message);
        return {};
    }

    char **argv = new char *[command.args.size() + 2];

    std::string executable = command.executable;
    std::vector<std::string> args = command.args;

    argv[0] = executable.data();

    for (unsigned i = 0; i < args.size(); i++) {
        argv[i + 1] = args[i].data();
    }

    argv[command.args.size() + 1] = NULL;

    if (pid == 0) {
        if (command.stdin_stream == Command::Stream::PIPE) {
            close(stdin_fd[1]);

            if (dup2(stdin_fd[0], 0) == -1) {
                error("`dup2` failed for stdin");
            }
        }

        if (command.stdout_stream == Command::Stream::PIPE) {
            close(stdout_fd[0]);

            if (dup2(stdout_fd[1], 1) == -1) {
                error("`dup2` failed for stdout");
            }
        }

        if (command.stderr_stream == Command::Stream::PIPE) {
            close(stderr_fd[0]);

            if (dup2(stderr_fd[1], 2) == -1) {
                error("`dup2` failed for stderr");
            }
        }

        if (execvp(executable.data(), argv) == -1) {
            std::string message(strerror(errno));
            error("failed to spawn process: " + message);
        }

        ASSERT_UNREACHABLE;
    } else {
        if (command.stdin_stream == Command::Stream::PIPE) {
            close(stdin_fd[0]);
        }

        if (command.stdout_stream == Command::Stream::PIPE) {
            close(stdout_fd[1]);
        }

        if (command.stderr_stream == Command::Stream::PIPE) {
            close(stderr_fd[1]);
        }

        process.pid = pid;
        process.stdin_write_fd = stdin_fd[1];
        process.stdout_read_fd = stdout_fd[0];
        process.stderr_read_fd = stderr_fd[0];
        return process;
    }

    return {};
}

ProcessResult Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);

    std::string stdout_buffer = stdin_write_fd != -1 ? read_all(stdout_read_fd) : "";
    std::string stderr_buffer = stderr_read_fd != -1 ? read_all(stderr_read_fd) : "";

    if (stdin_write_fd != -1 && close(stdin_write_fd) == -1) {
        error("`close` failed for stdin write fd");
    }

    if (stdout_read_fd != -1 && close(stdout_read_fd) == -1) {
        error("`close` failed for stdout read fd");
    }

    if (stderr_read_fd != -1 && close(stderr_read_fd) == -1) {
        error("`close` failed for stderr read fd");
    }

    return ProcessResult{
        .exit_code = WEXITSTATUS(status),
        .stdout_buffer = std::move(stdout_buffer),
        .stderr_buffer = std::move(stderr_buffer),
    };
}

std::string Process::read_all(int fd) {
    std::string result;
    std::string buffer(4096, '\0');

    while (true) {
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());

        if (bytes_read <= 0) {
            break;
        }

        result += std::string_view(buffer).substr(0, bytes_read);
    }

    return result;
}

} // namespace cli
} // namespace banjo
