#include "process.hpp"

#include "common.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>

namespace banjo {
namespace cli {

std::optional<Process> Process::spawn(const Command &command) {
    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

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
        CreateProcess(NULL, command_line.data(), NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, &process_info);

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

    if (result) {
        return Process(process_info.hProcess, process_info.hThread);
    } else {
        return {};
    }
}

Process::Process(HANDLE process, HANDLE thread) : process(process), thread(thread) {}

ProcessResult Process::wait() {
    WaitForSingleObject(process, INFINITE);

    DWORD exit_code;

    if (!GetExitCodeProcess(process, &exit_code)) {
        error("failed to get exit code of subprocess");
    }

    CloseHandle(process);
    CloseHandle(thread);

    return ProcessResult{
        .exit_code = static_cast<int>(exit_code),
    };
}

} // namespace cli
} // namespace banjo
