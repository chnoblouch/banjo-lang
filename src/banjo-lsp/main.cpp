#include "banjo/config/config_parser.hpp"

#include "server.hpp"

#include <chrono>
#include <thread>

int main(int argc, char *argv[]) {
    // std::this_thread::sleep_for(std::chrono::seconds(5));

    banjo::Config::instance() = banjo::ConfigParser().parse(argc, argv);
    banjo::lsp::Server().start();
    return 0;
}
