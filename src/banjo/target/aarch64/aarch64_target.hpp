#ifndef BANJO_TARGET_AARCH64_TARGET_H
#define BANJO_TARGET_AARCH64_TARGET_H

#include "banjo/target/aarch64/aarch64_reg_analyzer.hpp"
#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_data_layout.hpp"

#include <memory>

namespace banjo::target {

class AArch64Target final : public Target {

private:
    StandardDataLayout data_layout;
    AArch64RegAnalyzer reg_analyzer;

public:
    AArch64Target(TargetDescription descr, CodeModel code_model);

    TargetDataLayout &get_data_layout() override { return data_layout; }

    codegen::SSALowerer *create_ssa_lowerer() override;
    std::vector<std::unique_ptr<codegen::MachinePass>> create_passes() override;
    std::string get_output_file_ext() override;
    codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream) override;
};

} // namespace banjo::target

#endif
