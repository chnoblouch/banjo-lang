#include "banjo/target/x86_64/x86_64_encoder.hpp"
#include "banjo/emit/nasm_emitter.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace banjo;
using namespace target;
using namespace mcode;
using namespace codegen;

#define IMMEDIATE(value, size) Operand::from_immediate(std::to_string(value), size)
#define ADDRESS_BOS(base, offset, scale, size) Operand::from_indirect_addr({base, offset, scale}, size)
#define ADDRESS_B(base, size) Operand::from_indirect_addr({base}, size)

const Register RAX = Register::from_physical(X8664Register::RAX);
const Register RSP = Register::from_physical(X8664Register::RSP);
const std::vector<std::uint8_t> SIZES = {1, 2, 4, 8};

bool passed = true;

void print_encoding(const std::vector<std::uint8_t> &encoding) {
    for (unsigned i = 0; i < encoding.size(); i++) {
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)encoding[i];
        if (i != encoding.size() - 1) {
            std::cout << " ";
        }
    }
}

void report_error(const std::vector<std::uint8_t> &encoding, const std::vector<std::uint8_t> &expected_encoding) {
    std::cerr << "  expected : ";
    print_encoding(expected_encoding);
    std::cerr << "\n  result   : ";
    print_encoding(encoding);
    std::cerr << std::endl;
    passed = false;
}

void test_instr(std::string source, Instruction instr, std::vector<std::uint8_t> expected_encoding) {
    Module module_;
    Function *func = new Function("main", nullptr);
    module_.add(func);

    BasicBlock basic_block("", func);
    basic_block.append(instr);
    func->get_basic_blocks().append(basic_block);

    BinModule encoded_module = target::X8664Encoder().encode(module_);
    std::vector<std::uint8_t> encoding = encoded_module.text.get_data();

    std::cout << source << std::string(40 - source.size(), ' ');
    if (encoding != expected_encoding) {
        std::cout << "ERROR\n";
        report_error(encoding, expected_encoding);
    } else {
        std::cout << "OK" << std::endl;
    }
}

void assemble_instr(const Instruction &instr) {
    std::string source_file_name = "assembly.asm";
    std::string binary_file_name = "binary.bin";

    std::ofstream source_file(source_file_name);
    source_file << "bits 64\n";
    source_file << "default rel\n";

    Module dummy_module;
    BasicBlock dummy_block("", nullptr);
    dummy_block.append(instr);

    TargetDescription target(Architecture::X86_64, OperatingSystem::WINDOWS);
    std::stringstream source;
    NASMEmitter(dummy_module, source, target).emit_instr(dummy_block, *dummy_block.begin());

    source_file << source.str();
    source_file << std::endl;
    source_file.close();

    std::string command = "nasm -fbin -o" + binary_file_name + " " + source_file_name;
    system(command.c_str());

    std::ifstream stream(binary_file_name);
    stream.seekg(0, std::ios::end);
    std::istream::pos_type size = stream.tellg();
    std::vector<char> buffer;
    buffer.resize(size);
    stream.seekg(0, std::ios::beg);
    stream.read(buffer.data(), size);

    test_instr(source.str(), instr, std::vector<std::uint8_t>(buffer.begin(), buffer.end()));
}

Operand random_gp_reg(unsigned size) {
    Register reg = Register::from_physical(X8664Register::RAX + std::rand() % 16);
    return Operand::from_register(reg, size);
}

Operand random_sse_reg() {
    Register reg = Register::from_physical(X8664Register::XMM0 + std::rand() % 16);
    return Operand::from_register(reg);
}

Operand random_addr(unsigned size = 0) {
    Register reg = Register::from_physical(X8664Register::RAX + std::rand() % 16);
    return Operand::from_addr(IndirectAddress(reg), size);
}

Operand rax(unsigned size) {
    Register reg = Register::from_physical(X8664Register::RAX);
    return Operand::from_register(reg, size);
}

Operand rcx(unsigned size) {
    Register reg = Register::from_physical(X8664Register::RCX);
    return Operand::from_register(reg, size);
}

Operand imm(unsigned size) {
    if (size == 1) return Operand::from_int_immediate(100, size);
    else if (size == 2) return Operand::from_int_immediate(1000, size);
    else if (size == 4) return Operand::from_int_immediate(100000, size);
    else if (size == 8) return Operand::from_int_immediate(10000000000, size);
    else return Operand::from_int_immediate(0, size);
}

void test(Opcode opcode) {
    assemble_instr({opcode, {}});
}

void test_r(Opcode opcode, unsigned size) {
    assemble_instr({opcode, {random_gp_reg(size)}});
}

void test_rm(Opcode opcode, unsigned size) {
    assemble_instr({opcode, {random_gp_reg(size)}});
    assemble_instr({opcode, {random_addr(size)}});
}

void test_r_r(Opcode opcode, unsigned size) {
    assemble_instr({opcode, {random_gp_reg(size), random_gp_reg(size)}});
}

void test_r_rm(Opcode opcode, unsigned size) {
    assemble_instr({opcode, {random_gp_reg(size), random_gp_reg(size)}});
    assemble_instr({opcode, {random_gp_reg(size), random_addr(size)}});
}

void test_rm_rm(Opcode opcode, unsigned size) {
    assemble_instr({opcode, {random_gp_reg(size), random_gp_reg(size)}});
    assemble_instr({opcode, {random_gp_reg(size), random_addr(size)}});
    assemble_instr({opcode, {random_addr(size), random_gp_reg(size)}});
}

void test_rm_rmi(Opcode opcode) {
    assemble_instr({opcode, {rax(1), imm(1)}});
    assemble_instr({opcode, {rax(2), imm(1)}});
    assemble_instr({opcode, {rax(2), imm(2)}});
    assemble_instr({opcode, {rax(4), imm(1)}});
    assemble_instr({opcode, {rax(4), imm(2)}});
    assemble_instr({opcode, {rax(4), imm(4)}});
    assemble_instr({opcode, {rax(8), imm(1)}});
    assemble_instr({opcode, {rax(8), imm(2)}});
    assemble_instr({opcode, {rax(8), imm(4)}});

    for (unsigned size : SIZES) {
        assemble_instr({opcode, {random_gp_reg(size), imm(size == 8 ? 4 : size)}});
    }

    for (unsigned size : SIZES) {
        assemble_instr({opcode, {random_gp_reg(size), imm(1)}});
    }

    for (unsigned size : SIZES) {
        assemble_instr({opcode, {random_gp_reg(size), random_gp_reg(size)}});
        assemble_instr({opcode, {random_addr(size), random_gp_reg(size)}});
        assemble_instr({opcode, {random_gp_reg(size), random_addr(size)}});
    }
}

void test_sse_r_r(Opcode opcode) {
    assemble_instr({opcode, {random_sse_reg(), random_sse_reg()}});
}

void test_sse_r_rm(Opcode opcode) {
    assemble_instr({opcode, {random_sse_reg(), random_sse_reg()}});
    assemble_instr({opcode, {random_sse_reg(), random_addr()}});
}

void test_sse_rm_rm(Opcode opcode) {
    assemble_instr({opcode, {random_sse_reg(), random_sse_reg()}});
    assemble_instr({opcode, {random_sse_reg(), random_addr()}});
    assemble_instr({opcode, {random_addr(), random_sse_reg()}});
}

void test_mov() {
    for (unsigned size : SIZES) {
        assemble_instr({X8664Opcode::MOV, {random_gp_reg(size), imm(size)}});
    }

    for (unsigned size : SIZES) {
        assemble_instr({X8664Opcode::MOV, {random_gp_reg(size), random_gp_reg(size)}});
        assemble_instr({X8664Opcode::MOV, {random_addr(size), random_gp_reg(size)}});
        assemble_instr({X8664Opcode::MOV, {random_gp_reg(size), random_addr(size)}});
    }
}

void test_movsx() {
    for (auto pair : std::vector<std::pair<double, double>>{{2, 1}, {4, 1}, {8, 1}, {4, 2}, {8, 2}, {8, 4}}) {
        assemble_instr({X8664Opcode::MOVSX, {random_gp_reg(pair.first), random_gp_reg(pair.second)}});
        assemble_instr({X8664Opcode::MOVSX, {random_gp_reg(pair.first), random_addr(pair.second)}});
    }
}

void test_movzx() {
    assemble_instr({X8664Opcode::MOVZX, {random_gp_reg(2), random_gp_reg(1)}});
    assemble_instr({X8664Opcode::MOVZX, {random_gp_reg(4), random_gp_reg(1)}});
    assemble_instr({X8664Opcode::MOVZX, {random_gp_reg(8), random_gp_reg(1)}});
    assemble_instr({X8664Opcode::MOVZX, {random_gp_reg(4), random_gp_reg(2)}});
    assemble_instr({X8664Opcode::MOVZX, {random_gp_reg(8), random_gp_reg(2)}});
}

void test_shift(Opcode opcode) {
    for (unsigned size : SIZES) {
        assemble_instr({opcode, {random_gp_reg(size), Operand::from_int_immediate(1, 1)}});
        assemble_instr({opcode, {random_gp_reg(size), rcx(1)}});
        assemble_instr({opcode, {random_gp_reg(size), imm(1)}});

        assemble_instr({opcode, {random_addr(size), Operand::from_int_immediate(1, 1)}});
        assemble_instr({opcode, {random_addr(size), rcx(1)}});
        assemble_instr({opcode, {random_addr(size), imm(1)}});
    }
}

int main(int, char *[]) {
    std::srand(std::chrono::system_clock::now().time_since_epoch().count());

    test_mov();
    test_movsx();
    test_movzx();

    test_r(X8664Opcode::PUSH, 8);
    test_r(X8664Opcode::POP, 8);

    test_rm_rmi(X8664Opcode::ADD);
    test_rm_rmi(X8664Opcode::SUB);

    for (unsigned size : std::vector<unsigned>{2, 4, 8}) {
        test_r_r(X8664Opcode::IMUL, size);
    }

    for (unsigned size : SIZES) {
        test_rm(X8664Opcode::IDIV, size);
    }

    test_rm_rmi(X8664Opcode::AND);
    test_rm_rmi(X8664Opcode::OR);
    test_rm_rmi(X8664Opcode::XOR);

    test_shift(X8664Opcode::SHL);
    test_shift(X8664Opcode::SHR);

    test(X8664Opcode::CDQ);
    test(X8664Opcode::CQO);

    for (unsigned size : std::vector<unsigned>{2, 4, 8}) {
        test_r_rm(X8664Opcode::CMOVE, size);
        test_r_rm(X8664Opcode::CMOVNE, size);
        test_r_rm(X8664Opcode::CMOVA, size);
        test_r_rm(X8664Opcode::CMOVAE, size);
        test_r_rm(X8664Opcode::CMOVB, size);
        test_r_rm(X8664Opcode::CMOVBE, size);
        test_r_rm(X8664Opcode::CMOVG, size);
        test_r_rm(X8664Opcode::CMOVGE, size);
        test_r_rm(X8664Opcode::CMOVL, size);
        test_r_rm(X8664Opcode::CMOVLE, size);
    }

    test(X8664Opcode::RET);

    test_sse_rm_rm(X8664Opcode::MOVSS);
    test_sse_rm_rm(X8664Opcode::MOVSD);
    test_sse_rm_rm(X8664Opcode::MOVAPS);
    test_sse_rm_rm(X8664Opcode::MOVUPS);
    test_sse_r_rm(X8664Opcode::ADDSS);
    test_sse_r_rm(X8664Opcode::ADDSD);
    test_sse_r_rm(X8664Opcode::SUBSS);
    test_sse_r_rm(X8664Opcode::SUBSD);
    test_sse_r_rm(X8664Opcode::MULSS);
    test_sse_r_rm(X8664Opcode::MULSD);
    test_sse_r_rm(X8664Opcode::DIVSS);
    test_sse_r_rm(X8664Opcode::DIVSD);
    test_sse_r_rm(X8664Opcode::XORPS);
    test_sse_r_rm(X8664Opcode::XORPD);
    test_sse_r_rm(X8664Opcode::MINSS);
    test_sse_r_rm(X8664Opcode::MAXSS);
    test_sse_r_rm(X8664Opcode::SQRTSS);

    assemble_instr({X8664Opcode::CVTSS2SD, {random_sse_reg(), random_sse_reg()}});
    assemble_instr({X8664Opcode::CVTSS2SD, {random_sse_reg(), random_addr(4)}});

    assemble_instr({X8664Opcode::CVTSD2SS, {random_sse_reg(), random_sse_reg()}});
    assemble_instr({X8664Opcode::CVTSD2SS, {random_sse_reg(), random_addr(8)}});

    assemble_instr({X8664Opcode::CVTSI2SS, {random_sse_reg(), random_gp_reg(4)}});
    assemble_instr({X8664Opcode::CVTSI2SS, {random_sse_reg(), random_addr(4)}});

    assemble_instr({X8664Opcode::CVTSI2SD, {random_sse_reg(), random_gp_reg(8)}});
    assemble_instr({X8664Opcode::CVTSI2SD, {random_sse_reg(), random_addr(8)}});

    assemble_instr({X8664Opcode::CVTSS2SI, {random_gp_reg(4), random_sse_reg()}});
    assemble_instr({X8664Opcode::CVTSS2SI, {random_gp_reg(4), random_addr(4)}});

    assemble_instr({X8664Opcode::CVTSD2SI, {random_gp_reg(8), random_sse_reg()}});
    assemble_instr({X8664Opcode::CVTSD2SI, {random_gp_reg(8), random_addr(8)}});

    for (unsigned base = 0; base < 16; base++) {
        for (unsigned index = 0; index < 16; index++) {
            if (index == X8664Register::RSP) {
                continue;
            }

            for (unsigned scale : std::vector<unsigned>{1, 2, 4, 8}) {
                Register base_reg = Register::from_physical(base);
                Register index_reg = Register::from_physical(index);
                Operand addr = Operand::from_addr({base_reg, index_reg, (int)scale});
                assemble_instr({X8664Opcode::MOV, {random_gp_reg(8), addr}});
            }
        }
    }

    return passed ? 0 : 1;
}
