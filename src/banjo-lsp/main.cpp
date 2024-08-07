#include "banjo/config/config_parser.hpp"
#include "server.hpp"

#include <filesystem>

int main(int argc, char *argv[]) {
    using namespace banjo;

    lang::Config::instance() = lang::ConfigParser().parse(argc, argv);

    lang::Config::instance().paths.push_back("src");

    if (std::filesystem::is_directory("packages")) {
        for (std::filesystem::path path : std::filesystem::directory_iterator("packages")) {
            lang::Config::instance().paths.push_back(path / "src");
        }
    }

    lsp::Server().start();
    return 0;
}
