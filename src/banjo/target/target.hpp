#ifndef TARGET_H
#define TARGET_H

#include "codegen/machine_pass.hpp"
#include "emit/emitter.hpp"
#include "ir/calling_conv.hpp"
#include "target/target_data_layout.hpp"
#include "target/target_description.hpp"
#include "target/target_reg_analyzer.hpp"

#include <vector>

namespace codegen {
class IRLowerer;
} // namespace codegen

namespace target {

enum class CodeModel { SMALL, LARGE };

class Target {

protected:
    const TargetDescription descr;
    const CodeModel code_model;

protected:
    Target(TargetDescription descr, CodeModel code_model);

public:
    virtual ~Target() = default;

    const TargetDescription &get_descr() const { return descr; }
    CodeModel get_code_model() const { return code_model; }

    virtual TargetDataLayout &get_data_layout() = 0;
    virtual TargetRegAnalyzer &get_reg_analyzer() = 0;

    virtual codegen::IRLowerer *create_ir_lowerer() = 0;
    virtual std::vector<codegen::MachinePass *> create_pre_passes() = 0;
    virtual std::vector<codegen::MachinePass *> create_post_passes() = 0;
    virtual std::string get_output_file_ext() = 0;
    virtual codegen::Emitter *create_emitter(mcode::Module &module, std::ostream &stream) = 0;
    ir::CallingConv get_default_calling_conv();

    static Target *create(TargetDescription descr, CodeModel code_model);
};

} // namespace target

#endif
