#ifndef BANJO_TARGET_AARCH64_TARGET_H
#define BANJO_TARGET_AARCH64_TARGET_H

#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target.hpp"

namespace banjo {

namespace target {

class AArch64Target : public Target {

private:
    StandardDataLayout data_layout = StandardDataLayout(8, ssa::Primitive::I64);
    AArch64RegAnalyzer reg_analyzer;

public:
    AArch64Target(TargetDescription descr, CodeModel code_model);

    TargetDataLayout &get_data_layout() { return data_layout; }
    AArch64RegAnalyzer &get_reg_analyzer() { return reg_analyzer; }

    codegen::SSALowerer *create_ssa_lowerer();
    std::vector<codegen::MachinePass *> create_pre_passes();
    std::vector<codegen::MachinePass *> create_post_passes();
    std::string get_output_file_ext();
    codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream);
};

} // namespace target

} // namespace banjo

#endif
