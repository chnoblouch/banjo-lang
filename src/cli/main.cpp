#include "banjo/utils/cli_setup.hpp"

#include "cli.hpp"

int main(int argc, const char *argv[]) {
    banjo::utils::cli_setup();
    banjo::cli::CLI{}.run(argc, argv);
    return 0;
}
