#ifndef BANJO_TARGET_AARCH64_AARCH64_STACK_OFFSET_FIXUP_PASS_H
#define BANJO_TARGET_AARCH64_AARCH64_STACK_OFFSET_FIXUP_PASS_H

#include "banjo/codegen/machine_pass.hpp"

namespace banjo {
namespace target {

class AArch64StackOffsetFixupPass final : public codegen::MachinePass {

public:
    void run(mcode::Module &module_);

private:
    void run(mcode::BasicBlock &block);
};

} // namespace target
} // namespace banjo

#endif
