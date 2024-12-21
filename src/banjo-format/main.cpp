#include "formatter.hpp"

#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    std::string file(argv[1]);
    banjo::format::Formatter().format(file);

    return 0;
}
