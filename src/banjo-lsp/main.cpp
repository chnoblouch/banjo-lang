#include "banjo/config/config_parser.hpp"
#include "banjo/utils/cli_setup.hpp"

#include "server.hpp"

#include <chrono>
#include <thread>

int main(int argc, char *argv[]) {
    banjo::utils::cli_setup();

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    banjo::Config::instance() = banjo::ConfigParser().parse(argc, argv);
    banjo::lsp::Server().start();
    return 0;
}
