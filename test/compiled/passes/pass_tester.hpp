#ifndef PASS_TESTER_H
#define PASS_TESTER_H

#include "banjo/ir/module.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/target/target.hpp"

#include "banjo/target/x86_64/x86_64_target.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace banjo {

class PassTester {

private:
    typedef std::unordered_map<ir::VirtualRegister, ir::VirtualRegister> VRegMap;

    const std::string pass_name;
    const std::string &file_name;

    target::X8664Target arch_descr{
        target::TargetDescription(target::Architecture::X86_64, target::OperatingSystem::WINDOWS),
        target::CodeModel::LARGE,
    };

public:
    PassTester(const std::string &pass_name, const std::string &file_name);
    int run();

private:
    bool run(const std::filesystem::path &file_path);
    std::string run_pass(ir::Module &mod);
    bool compare_funcs(ir::Function *func_a, ir::Function *func_b);
    VRegMap create_vreg_map(ir::Function *func_a, ir::Function *func_b);
    bool compare_blocks(ir::BasicBlock &block_a, ir::BasicBlock &block_b, const VRegMap &vreg_map);
    bool compare_instrs(ir::Instruction &instr_a, ir::Instruction &instr_b, const VRegMap &vreg_map);
    bool compare_operands(ir::Operand &operand_a, ir::Operand &operand_b, const VRegMap &vreg_map);
    bool fail(const std::string &message);
};

} // namespace banjo

#endif
