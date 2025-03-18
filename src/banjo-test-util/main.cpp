#include "assembly_util.hpp"

#include "banjo/utils/write_buffer.hpp"

#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    banjo::WriteBuffer data = banjo::test::AssemblyUtil().assemble(argv[1]);

    for (unsigned i = 0; i < data.get_size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase;
        std::cout << static_cast<unsigned>(data.get_data()[i]);
    }

    std::cout << std::flush;

    return 0;
}
