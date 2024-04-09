#include "compiler.hpp"

#include "config/argument_parser.hpp"
#include "config/config_parser.hpp"
#include "config/standard_lib.hpp"
#include "utils/platform.hpp"
#include "utils/timing.hpp"

#ifdef OS_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#endif

#include <iostream>

int main(int argc, char *argv[]) {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    lang::ArgumentParser arg_parser;
    arg_parser.add_flag("version");
    arg_parser.add_flag("timing");
    lang::ParsedArgs args = arg_parser.parse(argc, argv);

    if (args.flags["version"]) {
        std::cout << BANJO_VERSION << std::endl;
        return 0;
    }

    PROFILE_SECTION_BEGIN("TOTAL");
    lang::Config::instance() = lang::ConfigParser().parse(argc, argv);
    lang::StandardLib::instance().discover();
    lang::Compiler(lang::Config::instance()).compile();
    PROFILE_SECTION_END("CLEANUP");
    PROFILE_SECTION_END("TOTAL");

    if (args.flags["timing"]) {
        ScopeTimer::dump_results();
    }

    return 0;
}
