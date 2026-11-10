#include "hot_reloader.hpp"

#include "banjo/config/config_parser.hpp"
#include "banjo/utils/cli_setup.hpp"

int main(int argc, char *argv[]) {
    banjo::utils::cli_setup();

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
