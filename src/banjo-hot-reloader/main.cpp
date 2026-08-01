#include "hot_reloader.hpp"

#include "banjo/config/config_parser.hpp"
#include "banjo/utils/platform.hpp"

#ifdef OS_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    banjo::Config::instance() = banjo::ConfigParser().parse(argc, argv);
    banjo::Config::instance().hot_reload = true;

    banjo::ArgumentParser arg_parser;
    arg_parser.add_value("executable", "");
    arg_parser.add_value("dir", "");
    banjo::ParsedArgs args = arg_parser.parse(argc, argv);

    banjo::hot_reloader::HotReloader hot_reloader;
    hot_reloader.run(args.values["executable"], args.values["dir"]);
    return 0;
}
