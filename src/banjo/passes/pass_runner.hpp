#ifndef BANJO_PASSES_PASS_RUNNER_H
#define BANJO_PASSES_PASS_RUNNER_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/target/target.hpp"

#include <vector>

namespace banjo {

namespace passes {

class PassRunner {

private:
    unsigned opt_level = 0;
    bool is_generate_addr_table = false;
    bool is_debug;

public:
    void run(ssa::Module &mod, target::Target *target);
    void set_opt_level(unsigned opt_level) { this->opt_level = opt_level; }
    void set_generate_addr_table(bool is_generate_addr_table) { this->is_generate_addr_table = is_generate_addr_table; }
    void set_debug(bool is_debug) { this->is_debug = is_debug; }

private:
    std::vector<Pass *> create_opt_passes(target::Target *target);
    void run_pass(Pass *pass, unsigned index, ssa::Module &mod);
};

} // namespace passes

} // namespace banjo

#endif
