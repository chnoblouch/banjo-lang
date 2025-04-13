#include "banjo/ssa/writer.hpp"
#include "banjo/utils/write_buffer.hpp"

#include "assembly_util.hpp"
#include "ssa_parser.hpp"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    if (strcmp(argv[1], "assemble") == 0) {
        banjo::WriteBuffer data = banjo::test::AssemblyUtil().assemble();

        for (unsigned i = 0; i < data.get_size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase;
            std::cout << static_cast<unsigned>(data.get_data()[i]);
        }

        std::cout.flush();
    } else if (strcmp(argv[1], "ssa") == 0) {
        banjo::ssa::Module ssa_mod = banjo::test::SSAParser().parse();
        std::ofstream out_stream("out.cryoir");
        banjo::ssa::Writer(out_stream).write(ssa_mod);
    }

    return 0;
}
