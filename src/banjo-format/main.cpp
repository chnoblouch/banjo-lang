#include "banjo/config/config.hpp"
#include "banjo/config/config_parser.hpp"
#include "banjo/format/formatter.hpp"
#include "banjo/source/source_file.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    std::string file_path = argv[argc - 1];
    banjo::lang::Config::instance() = banjo::lang::ConfigParser().parse(argc, argv);

    std::ifstream stream{file_path, std::ios::binary};
    std::unique_ptr<banjo::lang::SourceFile> file = banjo::lang::SourceFile::read({}, file_path, stream);
    banjo::lang::Formatter().format_in_place(*file);
    std::ofstream{file_path} << file->get_content();

    return 0;
}
