#ifndef PASSES_ADDR_TABLE_PASS_H
#define PASSES_ADDR_TABLE_PASS_H

#include "banjo/ir/addr_table.hpp"
#include "banjo/passes/pass.hpp"

#include <fstream>
#include <optional>

namespace banjo {

namespace passes {

class AddrTablePass : public Pass {

private:
    std::optional<AddrTable> addr_table;
    std::ofstream addr_table_file;
    unsigned next_symbol_index;

public:
    AddrTablePass(target::Target *target, std::optional<AddrTable> addr_table = {});
    void run(ir::Module &mod);

private:
    void add_symbol(const std::string &name, ir::Module &mod);
    void replace_uses(ir::Function *func);
    void replace_uses(ir::Function *func, ir::BasicBlock &basic_block);
};

} // namespace passes

} // namespace banjo

#endif
