#ifndef BANJO_PASSES_PIPELINE_H
#define BANJO_PASSES_PIPELINE_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/target/target.hpp"

#include <vector>

namespace banjo::passes {

class Pipeline {

public:
    struct Config {
        target::Target *target;
        unsigned opt_level = 0;
        bool generate_addr_table = false;
        bool debug;
    };

private:
    Config config;

public:
    Pipeline(Config config);
    void run(ssa::Module &mod);

private:
    std::vector<Pass *> create_opt_passes();
    void run_pass(Pass *pass, unsigned index, ssa::Module &mod);
};

} // namespace banjo::passes

#endif
