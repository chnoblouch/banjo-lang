#ifndef PASSES_ADDR_TABLE_PASS_H
#define PASSES_ADDR_TABLE_PASS_H

#include "banjo/passes/pass.hpp"

#include <fstream>

namespace banjo {

namespace passes {

class AddrTablePass : public Pass {

private:
    std::ofstream addr_table_file;
    unsigned next_symbol_index;

public:
    AddrTablePass(target::Target *target);
    void run(ir::Module &mod);

private:
    void add_symbol(const std::string &name, ir::Module &mod);
    void replace_uses(ir::Module &mod, ir::Function *func);
    void replace_uses(ir::Module &mod, ir::Function *func, ir::BasicBlock &basic_block);
};

} // namespace passes

} // namespace banjo

#endif
