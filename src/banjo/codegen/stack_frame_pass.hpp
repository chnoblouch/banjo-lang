#ifndef BANJO_CODEGEN_STACK_FRAME_PASS_H
#define BANJO_CODEGEN_STACK_FRAME_PASS_H

#include "banjo/codegen/machine_pass.hpp"
#include "banjo/target/target_reg_analyzer.hpp"

#include <unordered_map>

namespace banjo {

namespace codegen {

class StackFramePass : public MachinePass {

private:
    target::TargetRegAnalyzer &analyzer;
    mcode::Function *func;

public:
    StackFramePass(target::TargetRegAnalyzer &analyzer);
    void run(mcode::Module &module_);
    void run(mcode::Function *func);

private:
    void create_generic_region(int &generic_region_size, std::unordered_map<int, int> &pre_alloca_offsets, int top);
};

} // namespace codegen

} // namespace banjo

#endif
