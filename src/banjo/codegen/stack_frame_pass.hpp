#ifndef BANJO_CODEGEN_STACK_FRAME_PASS_H
#define BANJO_CODEGEN_STACK_FRAME_PASS_H

#include "banjo/codegen/machine_pass.hpp"

#include <unordered_map>

namespace banjo::codegen {

class StackFramePass : public MachinePass {

private:
    mcode::Function *func;

public:
    void run(mcode::Module &mod);
    void run(mcode::Function &func);

private:
    void create_generic_region(int &generic_region_size, std::unordered_map<int, int> &pre_alloca_offsets, int top);
};

} // namespace banjo::codegen

#endif
