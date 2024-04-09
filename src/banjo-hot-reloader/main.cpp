#include "hot_reloader.hpp"

#include "config/config_parser.hpp"
#include "config/standard_lib.hpp"
#include "utils/platform.hpp"

#ifdef OS_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
#endif

    lang::Config::instance() = lang::ConfigParser().parse(argc, argv);
    lang::Config::instance().set_hot_reload(true);
    lang::StandardLib::instance().discover();

    lang::ArgumentParser arg_parser;
    arg_parser.add_value("executable", "");
    arg_parser.add_value("dir", "");
    lang::ParsedArgs args = arg_parser.parse(argc, argv);

    HotReloader hot_reloader;
    hot_reloader.load(args.values["executable"]);
    hot_reloader.run(args.values["dir"]);
    return 0;
}
