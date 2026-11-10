#ifndef BANJO_UTILS_CLI_SETUP_H
#define BANJO_UTILS_CLI_SETUP_H

#include "banjo/utils/platform.hpp" // IWYU pragma: keep

#ifdef OS_WINDOWS
#    include <windows.h>
#endif

namespace banjo::utils {

inline void cli_setup() {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif
}

} // namespace banjo::utils

#endif
