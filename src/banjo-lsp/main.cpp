#include "banjo/config/config_parser.hpp"

#include "server.hpp"

#include <chrono>
#include <thread>

int main(int argc, char *argv[]) {
    using namespace banjo;

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    lang::Config::instance() = lang::ConfigParser().parse(argc, argv);
    lsp::Server().start();
    return 0;
}
