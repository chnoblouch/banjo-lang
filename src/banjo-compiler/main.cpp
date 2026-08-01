#include "compiler.hpp"

#include "banjo/config/argument_parser.hpp"
#include "banjo/config/config_parser.hpp"
#include "banjo/utils/platform.hpp"
#include "banjo/utils/timing.hpp"

#ifdef OS_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#endif

#include <iostream>

int main(int argc, char *argv[]) {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    banjo::ArgumentParser arg_parser;
    arg_parser.add_flag("version");
    arg_parser.add_flag("timing");
    banjo::ParsedArgs args = arg_parser.parse(argc, argv);

    if (args.flags["version"]) {
        std::cout << BANJO_VERSION << std::endl;
        return 0;
    }

    PROFILE_SECTION_BEGIN("TOTAL");
    banjo::Config::instance() = banjo::ConfigParser().parse(argc, argv);
    banjo::Compiler(banjo::Config::instance()).compile();
    PROFILE_SECTION_END("CLEANUP");
    PROFILE_SECTION_END("TOTAL");

    if (args.flags["timing"]) {
        banjo::ScopeTimer::dump_results();
    }

    return 0;
}
