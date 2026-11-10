#include "compiler.hpp"

#include "banjo/config/argument_parser.hpp"
#include "banjo/config/config_parser.hpp"
#include "banjo/utils/cli_setup.hpp"
#include "banjo/utils/timing.hpp"

int main(int argc, char *argv[]) {
    banjo::utils::cli_setup();

    banjo::ArgumentParser arg_parser;
    arg_parser.add_flag("timing");
    banjo::ParsedArgs args = arg_parser.parse(argc, argv);

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
