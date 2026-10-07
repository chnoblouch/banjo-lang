#include "x86_64_printer.hpp"

#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/macros.hpp"

#include <string_view>

namespace banjo::target {

// clang-format off
static const HashMap<mcode::Opcode, std::string_view> OPCODES{
    {target::X8664Opcode::MOV, "mov"},
    {target::X8664Opcode::PUSH, "push"},
    {target::X8664Opcode::POP, "pop"},
    {target::X8664Opcode::ADD, "add"},
    {target::X8664Opcode::SUB, "sub"},
    {target::X8664Opcode::IMUL, "imul"},
    {target::X8664Opcode::DIV, "div"},
    {target::X8664Opcode::IDIV, "idiv"},
    {target::X8664Opcode::AND, "and"},
    {target::X8664Opcode::OR, "or"},
    {target::X8664Opcode::XOR, "xor"},
    {target::X8664Opcode::SHL, "shl"},
    {target::X8664Opcode::SHR, "shr"},
    {target::X8664Opcode::SAR, "sar"},
    {target::X8664Opcode::CWD, "cwd"},
    {target::X8664Opcode::CDQ, "cdq"},
    {target::X8664Opcode::CQO, "cqo"},
    {target::X8664Opcode::XCHG, "xchg"},
    {target::X8664Opcode::LOCK_CMPXCHG, "lock cmpxchg"},
    {target::X8664Opcode::JMP, "jmp"},
    {target::X8664Opcode::CMP, "cmp"},
    {target::X8664Opcode::JE, "je"},
    {target::X8664Opcode::JNE, "jne"},
    {target::X8664Opcode::JA, "ja"},
    {target::X8664Opcode::JAE, "jae"},
    {target::X8664Opcode::JB, "jb"},
    {target::X8664Opcode::JBE, "jbe"},
    {target::X8664Opcode::JG, "jg"},
    {target::X8664Opcode::JGE, "jge"},
    {target::X8664Opcode::JL, "jl"},
    {target::X8664Opcode::JLE, "jle"},
    {target::X8664Opcode::SETE, "sete"},
    {target::X8664Opcode::SETNE, "setne"},
    {target::X8664Opcode::SETA, "seta"},
    {target::X8664Opcode::SETAE, "setae"},
    {target::X8664Opcode::SETB, "setb"},
    {target::X8664Opcode::SETBE, "setbe"},
    {target::X8664Opcode::SETG, "setg"},
    {target::X8664Opcode::SETGE, "setge"},
    {target::X8664Opcode::SETL, "setl"},
    {target::X8664Opcode::SETLE, "setle"},
    {target::X8664Opcode::CMOVE, "cmove"},
    {target::X8664Opcode::CMOVNE, "cmovne"},
    {target::X8664Opcode::CMOVA, "cmova"},
    {target::X8664Opcode::CMOVAE, "cmovae"},
    {target::X8664Opcode::CMOVB, "cmovb"},
    {target::X8664Opcode::CMOVBE, "cmovbe"},
    {target::X8664Opcode::CMOVG, "cmovg"},
    {target::X8664Opcode::CMOVGE, "cmovge"},
    {target::X8664Opcode::CMOVL, "cmovl"},
    {target::X8664Opcode::CMOVLE, "cmovle"},
    {target::X8664Opcode::CALL, "call"},
    {target::X8664Opcode::RET, "ret"},
    {target::X8664Opcode::LEA, "lea"},
    {target::X8664Opcode::MOVSX, "movsx"},
    {target::X8664Opcode::MOVZX, "movzx"},
    {target::X8664Opcode::MOVSS, "movss"},
    {target::X8664Opcode::MOVSD, "movsd"},
    {target::X8664Opcode::MOVAPS, "movaps"},
    {target::X8664Opcode::MOVUPS, "movups"},
    {target::X8664Opcode::MOVD, "movd"},
    {target::X8664Opcode::MOVQ, "movq"},
    {target::X8664Opcode::ADDSS, "addss"},
    {target::X8664Opcode::ADDSD, "addsd"},
    {target::X8664Opcode::SUBSS, "subss"},
    {target::X8664Opcode::SUBSD, "subsd"},
    {target::X8664Opcode::MULSS, "mulss"},
    {target::X8664Opcode::MULSD, "mulsd"},
    {target::X8664Opcode::DIVSS, "divss"},
    {target::X8664Opcode::DIVSD, "divsd"},
    {target::X8664Opcode::XORPS, "xorps"},
    {target::X8664Opcode::XORPD, "xorpd"},
    {target::X8664Opcode::MINSS, "minss"},
    {target::X8664Opcode::MINSD, "minsd"},
    {target::X8664Opcode::MAXSS, "maxss"},
    {target::X8664Opcode::MAXSD, "maxsd"},
    {target::X8664Opcode::SQRTSS, "sqrtss"},
    {target::X8664Opcode::SQRTSD, "sqrtsd"},
    {target::X8664Opcode::UCOMISS, "ucomiss"},
    {target::X8664Opcode::UCOMISD, "ucomisd"},
    {target::X8664Opcode::CVTSS2SD, "cvtss2sd"},
    {target::X8664Opcode::CVTSD2SS, "cvtsd2ss"},
    {target::X8664Opcode::CVTSI2SS, "cvtsi2ss"},
    {target::X8664Opcode::CVTSS2SI, "cvtss2si"},
    {target::X8664Opcode::CVTSI2SD, "cvtsi2sd"},
    {target::X8664Opcode::CVTSD2SI, "cvtsd2si"},

    {mcode::PseudoOpcode::EH_PUSHREG, ".eh_pushreg"},
    {mcode::PseudoOpcode::EH_ALLOCSTACK, ".eh_allocstack"},
};
// clang-format on

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_8{
    {target::X8664Register::RAX, "rax"},
    {target::X8664Register::RCX, "rcx"},
    {target::X8664Register::RDX, "rdx"},
    {target::X8664Register::RBX, "rbx"},
    {target::X8664Register::RSI, "rsi"},
    {target::X8664Register::RDI, "rdi"},
    {target::X8664Register::RSP, "rsp"},
    {target::X8664Register::RBP, "rbp"},
    {target::X8664Register::R8, "r8"},
    {target::X8664Register::R9, "r9"},
    {target::X8664Register::R10, "r10"},
    {target::X8664Register::R11, "r11"},
    {target::X8664Register::R12, "r12"},
    {target::X8664Register::R13, "r13"},
    {target::X8664Register::R14, "r14"},
    {target::X8664Register::R15, "r15"},
    {target::X8664Register::XMM0, "xmm0"},
    {target::X8664Register::XMM1, "xmm1"},
    {target::X8664Register::XMM2, "xmm2"},
    {target::X8664Register::XMM3, "xmm3"},
    {target::X8664Register::XMM4, "xmm4"},
    {target::X8664Register::XMM5, "xmm5"},
    {target::X8664Register::XMM6, "xmm6"},
    {target::X8664Register::XMM7, "xmm7"},
    {target::X8664Register::XMM8, "xmm8"},
    {target::X8664Register::XMM9, "xmm9"},
    {target::X8664Register::XMM10, "xmm10"},
    {target::X8664Register::XMM11, "xmm11"},
    {target::X8664Register::XMM12, "xmm12"},
    {target::X8664Register::XMM13, "xmm13"},
    {target::X8664Register::XMM14, "xmm14"},
    {target::X8664Register::XMM15, "xmm15"},
};
// clang-format on

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_4{
    {target::X8664Register::RAX, "eax"},
    {target::X8664Register::RCX, "ecx"},
    {target::X8664Register::RDX, "edx"},
    {target::X8664Register::RBX, "ebx"},
    {target::X8664Register::RSI, "esi"},
    {target::X8664Register::RDI, "edi"},
    {target::X8664Register::RSP, "esp"},
    {target::X8664Register::RBP, "ebp"},
    {target::X8664Register::R8, "r8d"},
    {target::X8664Register::R9, "r9d"},
    {target::X8664Register::R10, "r10d"},
    {target::X8664Register::R11, "r11d"},
    {target::X8664Register::R12, "r12d"},
    {target::X8664Register::R13, "r13d"},
    {target::X8664Register::R14, "r14d"},
    {target::X8664Register::R15, "r15d"},
    {target::X8664Register::XMM0, "xmm0"},
    {target::X8664Register::XMM1, "xmm1"},
    {target::X8664Register::XMM2, "xmm2"},
    {target::X8664Register::XMM3, "xmm3"},
    {target::X8664Register::XMM4, "xmm4"},
    {target::X8664Register::XMM5, "xmm5"},
    {target::X8664Register::XMM6, "xmm6"},
    {target::X8664Register::XMM7, "xmm7"},
    {target::X8664Register::XMM8, "xmm8"},
    {target::X8664Register::XMM9, "xmm9"},
    {target::X8664Register::XMM10, "xmm10"},
    {target::X8664Register::XMM11, "xmm11"},
    {target::X8664Register::XMM12, "xmm12"},
    {target::X8664Register::XMM13, "xmm13"},
    {target::X8664Register::XMM14, "xmm14"},
    {target::X8664Register::XMM15, "xmm15"},
};
// clang-format on

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_2{
    {target::X8664Register::RAX, "ax"},
    {target::X8664Register::RCX, "cx"},
    {target::X8664Register::RDX, "dx"},
    {target::X8664Register::RBX, "bx"},
    {target::X8664Register::RSI, "si"},
    {target::X8664Register::RDI, "di"},
    {target::X8664Register::RSP, "sp"},
    {target::X8664Register::RBP, "bp"},
    {target::X8664Register::R8, "r8w"},
    {target::X8664Register::R9, "r9w"},
    {target::X8664Register::R10, "r10w"},
    {target::X8664Register::R11, "r11w"},
    {target::X8664Register::R12, "r12w"},
    {target::X8664Register::R13, "r13w"},
    {target::X8664Register::R14, "r14w"},
    {target::X8664Register::R15, "r15w"},
};
// clang-format on

// clang-format off
static const HashMap<mcode::PhysicalReg, std::string_view> REGISTERS_1{
    {target::X8664Register::RAX, "al"},
    {target::X8664Register::RCX, "cl"},
    {target::X8664Register::RDX, "dl"},
    {target::X8664Register::RBX, "bl"},
    {target::X8664Register::RSI, "sil"},
    {target::X8664Register::RDI, "dil"},
    {target::X8664Register::RSP, "spl"},
    {target::X8664Register::RBP, "bpl"},
    {target::X8664Register::R8, "r8b"},
    {target::X8664Register::R9, "r9b"},
    {target::X8664Register::R10, "r10b"},
    {target::X8664Register::R11, "r11b"},
    {target::X8664Register::R12, "r12b"},
    {target::X8664Register::R13, "r13b"},
    {target::X8664Register::R14, "r14b"},
    {target::X8664Register::R15, "r15b"},
};
// clang-format on

void X8664Printer::print_opcode(mcode::Instruction &instr) {
    emit(OPCODES.find(instr.get_opcode()));
}

void X8664Printer::print_operand(mcode::Function &func, mcode::Instruction &instr, unsigned index) {
    mcode::Operand &operand = instr.get_operand(index);

    if (print_common_operand(func, operand)) {
        return;
    }

    bool requires_size = false;

    if (instr.get_operands().size() == 1) {
        requires_size = true;
    } else if (instr.get_operands().size() == 2) {
        if (index == 0 && instr.get_operand(1).is_int_immediate()) {
            requires_size = true;
        } else if (index == 1 && instr.get_operand(0).is_int_immediate()) {
            requires_size = true;
        }
    }

    if (requires_size) {
        print_size(operand.get_size());
        emit(' ');
    }

    if (operand.is_x86_64_addr()) {
        print_address(func, operand.get_x86_64_addr());
    } else if (operand.is_symbol_deref()) {
        print_symbol_deref(operand.get_deref_symbol());
    } else {
        ASSERT_UNREACHABLE;
    }
}

void X8664Printer::print_physical_reg(mcode::PhysicalReg reg, unsigned size) {
    switch (size) {
        case 1: emit(REGISTERS_1.find(reg)); break;
        case 2: emit(REGISTERS_2.find(reg)); break;
        case 4: emit(REGISTERS_4.find(reg)); break;
        case 8: emit(REGISTERS_8.find(reg)); break;
        default: ASSERT_UNREACHABLE;
    }
}

void X8664Printer::print_register(mcode::Register reg, unsigned size) {
    if (reg.is_virtual()) {
        print_virtual_reg(reg.get_virtual_reg(), size);
    } else if (reg.is_physical()) {
        print_physical_reg(reg.get_physical_reg(), size);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void X8664Printer::print_address(mcode::Function &func, const X8664Address &addr) {
    emit('[');

    if (addr.is_base_reg()) {
        print_register(addr.get_base_reg(), 8);
    } else if (addr.is_base_symbol()) {
        print_symbol(addr.get_base_symbol());
    } else {
        ASSERT_UNREACHABLE;
    }

    if (auto offset = addr.offset_reg) {
        emit(" + ");

        if (offset->scale != 1) {
            emit(offset->scale);
            emit(" * ");
        }

        print_register(offset->reg, 8);
    }

    if (addr.has_offset_imm()) {
        if (addr.get_offset_imm() != 0) {
            emit(" + ");
            emit(addr.get_offset_imm());
        }
    } else if (addr.has_offset_stack_addr()) {
        emit(" + ");
        print_stack_addr(func, addr.get_offset_stack_addr());
    }

    emit(']');
}

void X8664Printer::print_symbol_deref(const mcode::Symbol &symbol) {
    emit('[');
    print_symbol(symbol.name);
    emit(']');
}

void X8664Printer::print_size(unsigned size) {
    switch (size) {
        case 1: emit("byte"); break;
        case 2: emit("word"); break;
        case 4: emit("dword"); break;
        case 8: emit("qword"); break;
        default: ASSERT_UNREACHABLE;
    }
}

} // namespace banjo::target
