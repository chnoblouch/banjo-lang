#include "banjo/format/formatter.hpp"
#include "banjo/source/source_file.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    std::string file_path(argv[1]);

    std::ifstream stream{file_path};
    std::unique_ptr<banjo::lang::SourceFile> file = banjo::lang::SourceFile::read({}, file_path, stream);
    banjo::lang::Formatter().format(*file);

    // banjo::format::Formatter().format(file);

    return 0;
}
