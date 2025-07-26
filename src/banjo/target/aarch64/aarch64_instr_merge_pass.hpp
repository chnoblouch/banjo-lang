#ifndef BANJO_TARGET_AARCH64_INSTR_MERGE_PASS_H
#define BANJO_TARGET_AARCH64_INSTR_MERGE_PASS_H

#include "banjo/codegen/machine_pass.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace target {

class AArch64InstrMergePass : public codegen::MachinePass {

private:
    struct RegUsage {
        mcode::InstrIter producer;
        unsigned int num_consumers;
    };

    typedef std::unordered_map<int, RegUsage> RegUsageMap;

public:
    void run(mcode::Module &module_);
    void run(mcode::Function *func);
    void run(mcode::BasicBlock &basic_block);

private:
    std::unordered_map<int, RegUsage> analyze_usages(mcode::BasicBlock &basic_block);
    void merge_instrs(mcode::BasicBlock &basic_block, RegUsageMap &usages);
    void try_merge_str(mcode::Instruction &instr, RegUsageMap &usages);
    void try_merge_add(mcode::Instruction &instr, RegUsageMap &usages);
    AArch64Address try_merge_addr(const AArch64Address &addr, RegUsageMap &usages);
    void remove_useless_instrs(mcode::BasicBlock &basic_block, RegUsageMap &usages);
};

} // namespace target

} // namespace banjo

#endif
