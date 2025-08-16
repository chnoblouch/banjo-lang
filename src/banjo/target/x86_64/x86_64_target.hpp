#ifndef BANJO_TARGET_X86_64_TARGET
#define BANJO_TARGET_X86_64_TARGET

#include "banjo/target/standard_data_layout.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/x86_64/x86_64_reg_analyzer.hpp"

namespace banjo {

namespace target {

class X8664Target : public Target {

private:
    StandardDataLayout data_layout{8, ssa::Primitive::U64, true};
    X8664RegAnalyzer reg_analyzer;

public:
    X8664Target(TargetDescription descr, CodeModel code_model);

    TargetDataLayout &get_data_layout() { return data_layout; }
    TargetRegAnalyzer &get_reg_analyzer() { return reg_analyzer; }

    codegen::SSALowerer *create_ssa_lowerer();
    std::vector<codegen::MachinePass *> create_pre_passes();
    std::vector<codegen::MachinePass *> create_post_passes();
    std::string get_output_file_ext();
    codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream);
};

} // namespace target

} // namespace banjo

#endif
