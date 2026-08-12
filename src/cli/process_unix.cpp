#include "process.hpp"

#include "common.hpp"

#include "banjo/utils/macros.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace banjo::cli {

static char *copy_string(const std::string &string) {
    char *copy = new char[string.size() + 1];
    std::memcpy(copy, string.data(), string.size() + 1);
    return copy;
}

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

    char *executable = copy_string(command.executable);

    char **argv = new char *[command.args.size() + 2];
    argv[0] = copy_string(command.executable);

    for (unsigned i = 0; i < command.args.size(); i++) {
        argv[i + 1] = copy_string(command.args[i]);
    }

    argv[command.args.size() + 1] = NULL;

    pid_t pid = fork();

    if (pid == -1) {
        std::string message(strerror(errno));
        error("failed to spawn process: " + message);
        return {};
    }

    if (pid == 0) {
        if (command.stdin_stream == Command::Stream::PIPE) {
            if (close(stdin_fd[1]) == -1) {
                error("`close` failed for stdin pipe write end");
            }

            if (dup2(stdin_fd[0], 0) == -1) {
                error("`dup2` failed for stdin");
            }
        }

        if (command.stdout_stream == Command::Stream::PIPE) {
            if (close(stdout_fd[0]) == -1) {
                error("`close` failed for stdout pipe read end");
            }

            if (dup2(stdout_fd[1], 1) == -1) {
                error("`dup2` failed for stdout");
            }
        }

        if (command.stderr_stream == Command::Stream::PIPE) {
            if (close(stderr_fd[0]) == -1) {
                error("`close` failed for stderr pipe read end");
            }

            if (dup2(stderr_fd[1], 2) == -1) {
                error("`dup2` failed for stderr");
            }
        }

        if (execvp(executable, argv) == -1) {
            std::string message(strerror(errno));
            error("failed to spawn process: " + message);
        }

        ASSERT_UNREACHABLE;
    } else {
        delete executable;

        for (unsigned i = 0; i < command.args.size() + 1; i++) {
            delete argv[i];
        }

        if (command.stdin_stream == Command::Stream::PIPE) {
            if (close(stdin_fd[0]) == -1) {
                error("`close` failed for stdin read end");
            }
        }

        if (command.stdout_stream == Command::Stream::PIPE) {
            if (close(stdout_fd[1]) == -1) {
                error("`close` failed for stdout write end");
            }
        }

        if (command.stderr_stream == Command::Stream::PIPE) {
            if (close(stderr_fd[1]) == -1) {
                error("`close` failed for stderr write end");
            }
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
    std::string stdout_buffer;
    std::string stderr_buffer;

    struct pollfd poll_fds[2];
    poll_fds[0].fd = stdout_read_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = stderr_read_fd;
    poll_fds[1].events = POLLIN;

    while (poll_fds[0].fd != -1 || poll_fds[1].fd != -1) {
        int poll_result = poll(poll_fds, 2, -1);

        if (poll_result <= 0) {
            std::string message(strerror(errno));
            error("`poll` failed: " + message);
        }

        if (poll_fds[0].revents & POLLIN) {
            stdout_buffer += read_to_end(stdout_read_fd);
        }
        if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLIN)) {
            poll_fds[0].fd = -1;
        }

        if (poll_fds[1].revents & POLLIN) {
            stderr_buffer += read_to_end(stderr_read_fd);
        }
        if (poll_fds[1].revents & (POLLERR | POLLHUP | POLLIN)) {
            poll_fds[1].fd = -1;
        }
    }

    int status = 0;

    if (waitpid(pid, &status, 0) == -1) {
        std::string message(strerror(errno));
        error("failed to wait for process to terminate: " + message);
    }

    if (stdin_write_fd != -1 && close(stdin_write_fd) == -1) {
        error("`close` failed for stdin write fd");
    }

    if (stdout_read_fd != -1 && close(stdout_read_fd) == -1) {
        error("`close` failed for stdout read fd");
    }

    if (stderr_read_fd != -1 && close(stderr_read_fd) == -1) {
        error("`close` failed for stderr read fd");
    }

    int exit_code = WEXITSTATUS(status);

    // If the process was terminated by a signal, it probably crashed.
    if (WIFSIGNALED(status)) {
        exit_code = 1;
    }

    return ProcessResult{
        .exit_code = exit_code,
        .stdout_buffer = std::move(stdout_buffer),
        .stderr_buffer = std::move(stderr_buffer),
    };
}

std::string Process::read_to_end(int fd) {
    std::string result;
    std::string buffer(4096, '\0');

    while (true) {
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());

        if (bytes_read <= 0) {
            break;
        }

        result += std::string_view{&buffer[0], &buffer[bytes_read]};
    }

    return result;
}

} // namespace banjo::cli
