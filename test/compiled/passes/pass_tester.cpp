#include "pass_tester.hpp"

#include "banjo/ssa/writer.hpp"
#include "ir_parser.hpp"

#include "banjo/passes/inlining_pass.hpp"
#include "banjo/passes/peephole_optimizer.hpp"
#include "banjo/passes/sroa_pass.hpp"
#include "banjo/passes/stack_to_reg_pass.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace banjo {

PassTester::PassTester(const std::string &pass_name, const std::string &file_name)
  : pass_name(pass_name),
    file_name(file_name) {}

int PassTester::run() {
    return run(std::filesystem::path(TESTS_DIR) / pass_name / file_name) ? 0 : 1;
}

bool PassTester::run(const std::filesystem::path &file_path) {
    std::ifstream file(file_path);

    std::stringstream source_before;
    std::stringstream source_after;

    std::string line;
    bool reading_before = true;

    while (std::getline(file, line)) {
        if (line == "===") {
            reading_before = false;
        } else if (reading_before) {
            source_before << line << '\n';
        } else {
            source_after << line << '\n';
        }
    }

    std::stringstream source_before_copy(source_before.str());

    ssa::Module mod_before = IRParser(source_before).parse();
    ssa::Module mod_expected = IRParser(source_after).parse();

    ssa::Module mod_result = IRParser(source_before_copy).parse();
    std::string pass_logs = run_pass(mod_result);

    bool result = true;

    if (mod_result.get_functions().size() != mod_expected.get_functions().size()) {
        result = fail("number of functions doesn't match");
    } else {
        for (unsigned i = 0; i < mod_result.get_functions().size(); i++) {
            if (!compare_funcs(mod_result.get_functions()[i], mod_expected.get_functions()[i])) {
                result = false;
                break;
            }
        }
    }

    if (result) {
        std::cout << "OK" << std::endl;
    } else {
        std::filesystem::path report_path = std::filesystem::absolute(file_path.filename());

        std::ofstream report_file(report_path);
        report_file << "INPUT\n" << std::string(16, '-') << '\n';
        ssa::Writer(report_file).write(mod_before);
        report_file << "RESULT\n" << std::string(16, '-') << '\n';
        ssa::Writer(report_file).write(mod_result);
        report_file << "EXPECTED\n" << std::string(16, '-') << '\n';
        ssa::Writer(report_file).write(mod_expected);
        report_file << "LOGS\n" << std::string(16, '-') << '\n' << pass_logs;

        std::cout << " (" << report_path.string() << ")" << std::endl;
    }

    return result;
}

std::string PassTester::run_pass(ssa::Module &mod) {
    passes::Pass *pass = nullptr;

    if (pass_name == "sroa") pass = new passes::SROAPass(&arch_descr);
    else if (pass_name == "stack_to_reg") pass = new passes::StackToRegPass(&arch_descr);
    else if (pass_name == "inlining") pass = new passes::InliningPass(&arch_descr);

    std::stringstream logging_stream;
    pass->enable_logging(logging_stream);

    pass->run(mod);
    delete pass;

    return logging_stream.str();
}

bool PassTester::compare_funcs(ssa::Function *func_a, ssa::Function *func_b) {
    if (func_a->get_basic_blocks().get_size() != func_b->get_basic_blocks().get_size()) {
        return fail("number of basic blocks doesn't match");
    }

    ssa::BasicBlockIter block_a = func_a->begin();
    ssa::BasicBlockIter block_b = func_b->begin();

    VRegMap vreg_map = create_vreg_map(func_a, func_b);

    while (block_a != func_a->end()) {
        if (!compare_blocks(*block_a, *block_b, vreg_map)) {
            return false;
        }

        ++block_a;
        ++block_b;
    }

    return true;
}

PassTester::VRegMap PassTester::create_vreg_map(ssa::Function *func_a, ssa::Function *func_b) {
    VRegMap map;

    ssa::BasicBlockIter block_a = func_a->begin();
    ssa::BasicBlockIter block_b = func_b->begin();

    while (block_a != func_a->end()) {
        if (block_a->get_instrs().get_size() != block_b->get_instrs().get_size()) {
            return map;
        }

        ssa::InstrIter instr_a = block_a->begin();
        ssa::InstrIter instr_b = block_b->begin();

        while (instr_a != block_a->end()) {
            if (instr_a->get_dest().has_value() != instr_b->get_dest().has_value()) {
                return map;
            }

            if (instr_a->get_dest()) {
                map.insert({*instr_a->get_dest(), *instr_b->get_dest()});
            }

            ++instr_a;
            ++instr_b;
        }

        ++block_a;
        ++block_b;
    }

    return map;
}

bool PassTester::compare_blocks(ssa::BasicBlock &block_a, ssa::BasicBlock &block_b, const VRegMap &vreg_map) {
    if (block_a.get_instrs().get_size() != block_b.get_instrs().get_size()) {
        return fail("number of instructions doesn't match");
    }

    ssa::InstrIter instr_a = block_a.begin();
    ssa::InstrIter instr_b = block_b.begin();

    while (instr_a != block_a.end()) {
        if (!compare_instrs(*instr_a, *instr_b, vreg_map)) {
            return false;
        }

        ++instr_a;
        ++instr_b;
    }

    return true;
}

bool PassTester::compare_instrs(ssa::Instruction &instr_a, ssa::Instruction &instr_b, const VRegMap &vreg_map) {
    if (instr_a.get_opcode() != instr_b.get_opcode()) {
        return fail("opcode doesn't match");
    }

    if (instr_a.get_operands().size() != instr_b.get_operands().size()) {
        return fail("number of operands doesn't match");
    }

    if (instr_a.get_dest().has_value() != instr_b.get_dest().has_value()) {
        return fail("one instruction has a dst but the other doesn't");
    }

    for (unsigned i = 0; i < instr_a.get_operands().size(); i++) {
        if (!compare_operands(instr_a.get_operand(i), instr_b.get_operand(i), vreg_map)) {
            return false;
        }
    }

    return true;
}

bool PassTester::compare_operands(ssa::Operand &operand_a, ssa::Operand &operand_b, const VRegMap &vreg_map) {
    if (operand_a.is_register() && operand_b.is_register()) {
        return operand_b.get_register() == vreg_map.at(operand_a.get_register());
    }

    if (operand_a != operand_b) {
        return fail("operand mismatch");
    }

    return true;
}

bool PassTester::fail(const std::string &message) {
    std::cout << "FAILED: " << message;
    return false;
}

} // namespace banjo
