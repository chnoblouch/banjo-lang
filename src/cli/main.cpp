#include "banjo/utils/platform.hpp"

#include "cli.hpp"

#ifdef OS_WINDOWS
#    include <windows.h>
#else
#    include <sys/wait.h>
#endif

int main(int argc, const char *argv[]) {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    banjo::cli::CLI().run(argc, argv);
    return 0;
}
