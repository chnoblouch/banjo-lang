#ifndef LOWERING_TESTER_H
#define LOWERING_TESTER_H

#include "banjo/ssa/module.hpp"
#include "banjo/target/target.hpp"
#include "banjo/mcode/module.hpp"

#include <fstream>

class LoweringTester {

public:
    int run();

private:
    bool run(const std::filesystem::path &file_path, target::Target *arch_descr);
    ssa::Module load_ir_section(std::ifstream &stream);
    mcode::Module load_machine_section(std::ifstream &stream);

    void skip_whitespace(std::ifstream &stream);

    bool fail(const std::string &message);

};

#endif