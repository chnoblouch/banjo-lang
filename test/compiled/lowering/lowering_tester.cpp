#include "lowering_tester.hpp"

#include "banjo/target/x86_64/x86_64_target.hpp"
#include "banjo/target/x86_64/x86_64_ssa_lowerer.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

int LoweringTester::run() {
    std::cout << "test directory: " << TESTS_DIR << std::endl;
    std::filesystem::path tests_dir(TESTS_DIR);

    std::cout << "running tests..." << std::endl;
    target::X8664Target arch_descr(target::CodeModel::LARGE, lang::OperatingSystem::WINDOWS);

    std::vector<std::filesystem::path> test_files;
    for (const std::filesystem::path &test_file : std::filesystem::directory_iterator(tests_dir)) {
        test_files.push_back(test_file);
    }

    for (unsigned i = 0; i < test_files.size(); i++) {
        unsigned padding = std::to_string(test_files.size()).size();
        std::cout << std::setfill('0') << "[" << std::setw(padding) << i << " / " << test_files.size() << "] "; 

        const std::filesystem::path &test_file = test_files[i];
        std::cout << test_file.stem().string() << " ";
        run(test_file, &arch_descr);
    }

    return 0;
}

bool LoweringTester::run(const std::filesystem::path &file_path, target::Target *arch_descr) {
    std::ifstream stream(file_path);
    ssa::Module ir_module = load_ir_section(stream);
    gen::MachineModule m_module = load_machine_section(stream);
    gen::MachineModule lowered_module = target::X8664SSALowerer(arch_descr).lower_module(ir_module);

    if (m_module.get_functions().size() != lowered_module.get_functions().size()) {
        return fail("unequal number of functions");
    }

    std::cout << "\u001b[32mOK\u001b[0m" << std::endl;

    return true;
}

ssa::Module LoweringTester::load_ir_section(std::ifstream &stream) {
    ssa::Module ir_module;

    ssa::Function *cur_func = nullptr;

    std::string line;
    while (std::getline(stream, line)) {
        if (line.starts_with("func")) {
            cur_func = new ssa::Function("name", {}, ssa::Primitive::VOID, ssa::CallingConv::X86_64_MS_ABI);
            ir_module.add(cur_func);
        } else if (line.starts_with("  ")) {
            
        }
    }

    return ir_module;
}

gen::MachineModule LoweringTester::load_machine_section(std::ifstream &stream) {
    gen::MachineModule m_module;

    return m_module;
}

void LoweringTester::skip_whitespace(std::ifstream &stream) {
    char c = stream.peek();
    while (c == ' ' || c == '\t') {
        stream.get();
        c = stream.peek();
    }
}

bool LoweringTester::fail(const std::string &message) {
    std::cout << "\u001b[31mFAILED: " << message << "\u001b[0m";
    return false;
}
