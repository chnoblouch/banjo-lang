#include "process.hpp"

#include "common.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace banjo {
namespace cli {

std::optional<Process> Process::spawn(const Command &command) {
    Process process;

    SECURITY_ATTRIBUTES pipe_attrs;
    pipe_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipe_attrs.lpSecurityDescriptor = NULL;
    pipe_attrs.bInheritHandle = TRUE;

    HANDLE stdin_read_handle = NULL;
    process.stdin_write_handle = NULL;

    if (command.stdin_stream == Command::Stream::INHERIT) {
        stdin_read_handle = GetStdHandle(STD_INPUT_HANDLE);

        if (stdin_read_handle == INVALID_HANDLE_VALUE) {
            error("`GetStdHandle` failed");
        }
    } else if (command.stdin_stream == Command::Stream::PIPE) {
        if (!CreatePipe(&stdin_read_handle, &process.stdin_write_handle, &pipe_attrs, 0)) {
            error("`CreatePipe` failed");
        }

        if (!SetHandleInformation(process.stdin_write_handle, HANDLE_FLAG_INHERIT, 0)) {
            error("`SetHandleInformation` failed");
        }
    }

    process.stdout_read_handle = NULL;
    HANDLE stdout_write_handle = NULL;

    if (command.stdout_stream == Command::Stream::INHERIT) {
        stdout_write_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        if (stdout_write_handle == INVALID_HANDLE_VALUE) {
            error("`GetStdHandle` failed");
        }
    } else if (command.stdout_stream == Command::Stream::PIPE) {
        if (!CreatePipe(&process.stdout_read_handle, &stdout_write_handle, &pipe_attrs, 0)) {
            error("`CreatePipe` failed");
        }

        if (!SetHandleInformation(process.stdout_read_handle, HANDLE_FLAG_INHERIT, 0)) {
            error("`SetHandleInformation` failed");
        }
    }

    process.stderr_read_handle = NULL;
    HANDLE stderr_write_handle = NULL;

    if (command.stderr_stream == Command::Stream::INHERIT) {
        stderr_write_handle = GetStdHandle(STD_ERROR_HANDLE);

        if (stderr_write_handle == INVALID_HANDLE_VALUE) {
            error("`GetStdHandle` failed");
        }
    } else if (command.stderr_stream == Command::Stream::PIPE) {
        if (!CreatePipe(&process.stderr_read_handle, &stderr_write_handle, &pipe_attrs, 0)) {
            error("`CreatePipe` failed");
        }

        if (!SetHandleInformation(process.stderr_read_handle, HANDLE_FLAG_INHERIT, 0)) {
            error("`SetHandleInformation` failed");
        }
    }

    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    startup_info.hStdInput = stdin_read_handle;
    startup_info.hStdOutput = stdout_write_handle;
    startup_info.hStdError = stderr_write_handle;

    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(process_info));

    std::vector<std::string> command_components;
    command_components.push_back(command.executable);
    command_components.insert(command_components.end(), command.args.begin(), command.args.end());

    std::string command_line;

    for (unsigned i = 0; i < command_components.size(); i++) {
        if (i != 0) {
            command_line += " ";
        }

        command_line += "\"";

        for (char c : command_components[i]) {
            if (c == '\\') {
                command_line += "\\\\";
            } else if (c == '\"') {
                command_line += "\"";
            } else {
                command_line += c;
            }
        }

        command_line += "\"";
    }

    BOOL result =
        CreateProcess(NULL, command_line.data(), NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);

    if (!result) {
        DWORD winapi_error = GetLastError();
        LPSTR message_buffer = NULL;

        DWORD message_length = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            winapi_error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&message_buffer),
            0,
            NULL
        );

        std::string message(message_buffer, message_length - 3);
        LocalFree(message_buffer);

        error("failed to spawn process: " + message);
    }

    if (command.stdin_stream == Command::Stream::PIPE && !CloseHandle(stdin_read_handle)) {
        error("`CloseHandle` failed");
    }

    if (command.stdout_stream == Command::Stream::PIPE && !CloseHandle(stdout_write_handle)) {
        error("`CloseHandle` failed");
    }

    if (command.stderr_stream == Command::Stream::PIPE && !CloseHandle(stderr_write_handle)) {
        error("`CloseHandle` failed");
    }

    if (result) {
        process.process = process_info.hProcess;
        process.thread = process_info.hThread;
        return process;
    } else {
        return {};
    }
}

ProcessResult Process::wait() {
    WaitForSingleObject(process, INFINITE);

    DWORD exit_code;

    if (!GetExitCodeProcess(process, &exit_code)) {
        error("failed to get exit code of subprocess");
    }

    std::string stdout_buffer = stdout_read_handle ? read_all(stdout_read_handle) : "";
    std::string stderr_buffer = stderr_read_handle ? read_all(stderr_read_handle) : "";

    if (stdin_write_handle && !CloseHandle(stdin_write_handle)) {
        error("`CloseHandle` failed");
    }

    if (stdout_read_handle && !CloseHandle(stdout_read_handle)) {
        error("`CloseHandle` failed");
    }

    if (stderr_read_handle && !CloseHandle(stderr_read_handle)) {
        error("`CloseHandle` failed");
    }

    if (!CloseHandle(process)) {
        error("`CloseHandle` failed");
    }

    if (!CloseHandle(thread)) {
        error("`CloseHandle` failed");
    }

    return ProcessResult{
        .exit_code = static_cast<int>(exit_code),
        .stdout_buffer = std::move(stdout_buffer),
        .stderr_buffer = std::move(stderr_buffer),
    };
}

std::string Process::read_all(HANDLE file) {
    std::string result;
    std::string buffer(4096, '\0');

    while (true) {
        DWORD bytes_read;

        if (!ReadFile(file, buffer.data(), buffer.size(), &bytes_read, NULL)) {
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        result += std::string_view(buffer).substr(0, bytes_read);
    }

    return result;
}

} // namespace cli
} // namespace banjo
