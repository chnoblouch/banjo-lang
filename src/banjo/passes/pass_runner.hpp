#ifndef PASS_RUNNER_H
#define PASS_RUNNER_H

#include "ir/module.hpp"
#include "passes/pass.hpp"
#include "target/target.hpp"

#include <vector>

namespace passes {

class PassRunner {

private:
    unsigned opt_level = 0;
    bool is_generate_addr_table = false;
    bool is_debug;

public:
    void run(ir::Module &mod, target::Target *target);
    void set_opt_level(unsigned opt_level) { this->opt_level = opt_level; }
    void set_generate_addr_table(bool is_generate_addr_table) { this->is_generate_addr_table = is_generate_addr_table; }
    void set_debug(bool is_debug) { this->is_debug = is_debug; }

private:
    std::vector<Pass *> create_opt_passes(target::Target *target);
    void run_pass(Pass *pass, unsigned index, ir::Module &mod);
};

} // namespace passes

#endif
