#ifndef BANJO_TARGET_X86_64_TARGET
#define BANJO_TARGET_X86_64_TARGET

#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/target/x86_64/x86_64_reg_analyzer.hpp"

#include <memory>

namespace banjo::target {

class X8664Target final : public Target {

private:
    StandardDataLayout data_layout;
    X8664RegAnalyzer reg_analyzer;

public:
    X8664Target(TargetDescription descr, CodeModel code_model);

    TargetDataLayout &get_data_layout() override { return data_layout; }

    codegen::SSALowerer *create_ssa_lowerer() override;
    std::vector<std::unique_ptr<codegen::MachinePass>> create_passes() override;
    std::string get_output_file_ext() override;
    codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream) override;
};

} // namespace banjo::target

#endif
