#ifndef AARCH64_TARGET_H
#define AARCH64_TARGET_H

#include "target/aarch64/aarch64_reg_analyzer.hpp"
#include "target/standard_data_layout.hpp"
#include "target/target.hpp"

namespace target {

class AArch64Target : public Target {

private:
    StandardDataLayout data_layout = StandardDataLayout(8, ir::Type(ir::Primitive::I64));
    AArch64RegAnalyzer reg_analyzer;

public:
    AArch64Target(TargetDescription descr, CodeModel code_model);

    TargetDataLayout &get_data_layout() { return data_layout; }
    AArch64RegAnalyzer &get_reg_analyzer() { return reg_analyzer; }

    codegen::IRLowerer *create_ir_lowerer();
    std::vector<codegen::MachinePass *> create_pre_passes();
    std::vector<codegen::MachinePass *> create_post_passes();
    std::string get_output_file_ext();
    codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream);
};

} // namespace target

#endif
