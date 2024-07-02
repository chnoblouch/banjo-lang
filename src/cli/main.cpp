#include "utils/paths.hpp"
#include "utils/platform.hpp"

#include <cstdlib>

#ifdef OS_WINDOWS
#    include <windows.h>
#else
#    include <sys/wait.h>
#endif

int main(int argc, char *argv[]) {
    using namespace banjo;

#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::filesystem::path exec_path = Paths::executable();
    std::filesystem::path script_path = exec_path.parent_path().parent_path() / "scripts" / "cli" / "cli.py";

#ifdef OS_WINDOWS
    std::string command = "python ";
#else
    std::string command = "python3 ";
#endif

    command += script_path.string();

    for (unsigned i = 1; i < argc; i++) {
        command += " " + std::string(argv[i]);
    }

    int exit_code = system(command.c_str());

#ifdef OS_WINDOWS
    return exit_code;
#else
    return WEXITSTATUS(exit_code);
#endif
}
