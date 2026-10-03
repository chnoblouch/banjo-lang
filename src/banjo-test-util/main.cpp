
#include "banjo/target/target_description.hpp"
#include "banjo/utils/write_buffer.hpp"

#include "assembly_util.hpp"
#include "codegen_util.hpp"
#include "formatter_util.hpp"
#include "ssa_util.hpp"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

static banjo::target::Architecture parse_arch(std::string_view value) {
    if (value == "x86_64") {
        return banjo::target::Architecture::X86_64;
    } else if (value == "aarch64") {
        return banjo::target::Architecture::AARCH64;
    } else {
        std::cerr << "invalid arch '" + std::string{value} + "'\n";
        std::exit(EXIT_FAILURE);
    }
}

static std::string read_stdin() {
    return {std::istreambuf_iterator<char>{std::cin}, {}};
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    if (strcmp(argv[1], "format") == 0) {
        banjo::test::FormatterUtil{}.format(argv[2]);
    } else if (strcmp(argv[1], "assemble") == 0) {
        banjo::target::Architecture arch = parse_arch(argv[2]);
        std::string input = read_stdin();
        banjo::WriteBuffer data = banjo::test::AssemblyUtil{arch, input}.assemble();

        for (unsigned i = 0; i < data.get_size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase;
            std::cout << static_cast<unsigned>(data.get_data()[i]);
        }

        std::cout.flush();
    } else if (strcmp(argv[1], "ssa") == 0) {
        banjo::test::SSAUtil{}.optimize(argv[2]);
    } else if (strcmp(argv[1], "codegen") == 0) {
        banjo::target::Architecture arch = parse_arch(argv[2]);
        banjo::test::CodegenUtil{}.lower(arch);
    }

    return 0;
}
