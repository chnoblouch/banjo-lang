#include "banjo/config/config_parser.hpp"
#include "server.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

int main(int argc, char *argv[]) {
    using namespace banjo;

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    lang::Config::instance() = lang::ConfigParser().parse(argc, argv);

    lang::Config::instance().paths.push_back("src");
    std::filesystem::path packages_path = "packages";

    if (std::filesystem::is_directory(packages_path)) {
        for (const std::filesystem::path &package_path : std::filesystem::directory_iterator(packages_path)) {
            lang::Config::instance().paths.push_back(package_path / "src");
        }
    }

    lsp::Server().start();

    return 0;
}
