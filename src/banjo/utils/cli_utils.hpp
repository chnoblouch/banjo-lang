#ifndef BANJO_UTILS_CLI_UTILS_H
#define BANJO_UTILS_CLI_UTILS_H

#include "banjo/utils/platform.hpp" // IWYU pragma: keep

#ifdef OS_WINDOWS
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#elif OS_LINUX || OS_MACOS
#    include <unistd.h>
#endif

namespace banjo::utils {

inline void cli_setup() {
#if OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif
}

inline bool is_stdout_terminal() {
#if OS_WINDOWS
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD console_mode;
    return GetConsoleMode(handle, &console_mode);
#elif OS_LINUX || OS_MACOS
    return isatty(1);
#endif
}

} // namespace banjo::utils

#endif
