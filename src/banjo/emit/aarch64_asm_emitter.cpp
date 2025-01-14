#include "aarch64_asm_emitter.hpp"

#include "banjo/mcode/operand.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

#include <variant>

namespace banjo {

namespace codegen {

// clang-format off
const std::unordered_map<mcode::Opcode, std::string> AArch64AsmEmitter::OPCODE_NAMES = {
    {target::AArch64Opcode::MOV, "mov"},
    {target::AArch64Opcode::MOVZ, "movz"},
    {target::AArch64Opcode::MOVK, "movk"},
    {target::AArch64Opcode::LDR, "ldr"},
    {target::AArch64Opcode::LDRB, "ldrb"},
    {target::AArch64Opcode::LDRH, "ldrh"},
    {target::AArch64Opcode::STR, "str"},
    {target::AArch64Opcode::STRB, "strb"},
    {target::AArch64Opcode::STRH, "strh"},
    {target::AArch64Opcode::LDP, "ldp"},
    {target::AArch64Opcode::STP, "stp"},
    {target::AArch64Opcode::ADD, "add"},
    {target::AArch64Opcode::SUB, "sub"},
    {target::AArch64Opcode::MUL, "mul"},
    {target::AArch64Opcode::SDIV, "sdiv"},
    {target::AArch64Opcode::UDIV, "udiv"},
    {target::AArch64Opcode::AND, "and"},
    {target::AArch64Opcode::ORR, "orr"},
    {target::AArch64Opcode::EOR, "eor"},
    {target::AArch64Opcode::LSL, "lsl"},
    {target::AArch64Opcode::ASR, "asr"},
    {target::AArch64Opcode::CSEL, "csel"},
    {target::AArch64Opcode::FMOV, "fmov"},
    {target::AArch64Opcode::FADD, "fadd"},
    {target::AArch64Opcode::FSUB, "fsub"},
    {target::AArch64Opcode::FMUL, "fmul"},
    {target::AArch64Opcode::FDIV, "fdiv"},
    {target::AArch64Opcode::FCVT, "fcvt"},
    {target::AArch64Opcode::SCVTF, "scvtf"},
    {target::AArch64Opcode::UCVTF, "ucvtf"},
    {target::AArch64Opcode::FCVTZS, "fcvtzs"},
    {target::AArch64Opcode::FCVTZU, "fcvtzu"},
    {target::AArch64Opcode::FCSEL, "fcsel"},
    {target::AArch64Opcode::CMP, "cmp"},
    {target::AArch64Opcode::FCMP, "fcmp"},
    {target::AArch64Opcode::B, "b"},
    {target::AArch64Opcode::BR, "br"},
    {target::AArch64Opcode::B_EQ, "b.eq"},
    {target::AArch64Opcode::B_NE, "b.ne"},
    {target::AArch64Opcode::B_HS, "b.hs"},
    {target::AArch64Opcode::B_LO, "b.lo"},
    {target::AArch64Opcode::B_HI, "b.hi"},
    {target::AArch64Opcode::B_LS, "b.ls"},
    {target::AArch64Opcode::B_GE, "b.ge"},
    {target::AArch64Opcode::B_LT, "b.lt"},
    {target::AArch64Opcode::B_GT, "b.gt"},
    {target::AArch64Opcode::B_LE, "b.le"},
    {target::AArch64Opcode::BL, "bl"},
    {target::AArch64Opcode::BLR, "blr"},
    {target::AArch64Opcode::RET, "ret"},
    {target::AArch64Opcode::ADRP, "adrp"},
    {target::AArch64Opcode::SXTW, "sxtw"}
};
// clang-format on

AArch64AsmEmitter::AArch64AsmEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target)
  : Emitter(module, stream),
    target(target) {}

void AArch64AsmEmitter::generate() {
    PROFILE_SCOPE("clang asm emitter");

    if (target.is_darwin()) {
        symbol_prefix = "_";
    }

    for (const std::string &external_symbol : module.get_external_symbols()) {
        stream << ".extern " << symbol_prefix << external_symbol << "\n";
    }
    stream << "\n";

    for (const std::string &global_symbol : module.get_global_symbols()) {
        stream << ".global " << symbol_prefix << global_symbol << "\n";
    }
    stream << "\n";

    stream << ".text\n";
    for (mcode::Function *func : module.get_functions()) {
        emit_func(func);
    }

    stream << ".data\n";
    for (const mcode::Global &global : module.get_globals()) {
        emit_global(global);
    }
}

void AArch64AsmEmitter::emit_global(const mcode::Global &global) {
    stream << symbol_prefix << global.name << ": ";

    if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
        switch (global.size) {
            case 1: stream << ".byte"; break;
            case 2: stream << ".hword"; break;
            case 4: stream << ".word"; break;
            case 8: stream << ".xword"; break;
            default: ASSERT_UNREACHABLE;
        }

        stream << " " << value->to_string();
    } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
        stream << (global.size == 4 ? ".float " : ".double ") << value;
    } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
        stream << ".ascii \"" << std::hex;

        for (unsigned char c : *value) {
            stream << "\\x" << (unsigned)c;
        }

        stream << std::dec << "\"";
    } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
        stream << ".string \"";

        for (char c : *value) {
            if (c == '\0') stream << "\\0";
            else if (c == '\n') stream << "\\n";
            else if (c == '\r') stream << "\\r";
            else if (c == '\"') stream << "\\\"";
            else if (c == '\\') stream << "\\\\";
            else stream << c;
        }

        stream << "\"";
    } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
        switch (global.size) {
            case 1: stream << ".byte"; break;
            case 2: stream << ".hword"; break;
            case 4: stream << ".word"; break;
            case 8: stream << ".xword"; break;
            default: ASSERT_UNREACHABLE;
        }

        stream << " " << symbol_prefix << value->name;
    } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
        stream << ".zero " << global.size;
    } else {
        ASSERT_UNREACHABLE;
    }

    stream << "\n";
}

void AArch64AsmEmitter::emit_func(mcode::Function *func) {
    stream << symbol_prefix << func->get_name() << ":\n";

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        emit_basic_block(basic_block);
    }

    stream << "\n";
}

void AArch64AsmEmitter::emit_basic_block(mcode::BasicBlock &basic_block) {
    if (!basic_block.get_label().empty()) {
        stream << basic_block.get_label() << ":\n";
    }

    for (mcode::Instruction &instr : basic_block) {
        emit_instr(basic_block.get_func(), instr);
        stream << "\n";
    }

    stream << "\n";
}

void AArch64AsmEmitter::emit_instr(mcode::Function *func, mcode::Instruction &instr) {
    stream << "  " << OPCODE_NAMES.find(instr.get_opcode())->second;

    if (instr.get_operands().get_size() == 0) {
        return;
    }

    stream << " ";

    for (int i = 0; i < (int)instr.get_operands().get_size(); i++) {
        emit_operand(func, instr.get_operand(i));
        if (i != (int)instr.get_operands().get_size() - 1) {
            stream << ", ";
        }
    }
}

void AArch64AsmEmitter::emit_operand(mcode::Function *func, const mcode::Operand &operand) {
    if (operand.is_int_immediate()) stream << "#" << operand.get_int_immediate();
    else if (operand.is_fp_immediate()) stream << "#" << operand.get_fp_immediate();
    else if (operand.is_physical_reg()) emit_reg(operand.get_physical_reg(), operand.get_size());
    else if (operand.is_stack_slot()) emit_stack_slot(func, operand.get_stack_slot());
    else if (operand.is_symbol()) emit_symbol(operand.get_symbol());
    else if (operand.is_label()) stream << operand.get_label();
    else if (operand.is_aarch64_addr()) emit_addr(operand.get_aarch64_addr());
    else if (operand.is_stack_slot_offset()) emit_stack_slot_offset(func, operand.get_stack_slot_offset());
    else if (operand.is_aarch64_left_shift()) stream << "lsl #" << operand.get_aarch64_left_shift();
    else if (operand.is_aarch64_condition()) emit_condition(operand.get_aarch64_condition());
    else ASSERT_UNREACHABLE;
}

void AArch64AsmEmitter::emit_reg(int reg, int size) {
    if (reg >= target::AArch64Register::R0 && reg <= target::AArch64Register::R_LAST) {
        stream << (size == 8 ? "x" : "w") << (reg - target::AArch64Register::R0);
    } else if (reg >= target::AArch64Register::V0 && reg <= target::AArch64Register::V_LAST) {
        switch (size) {
            case 1: stream << "b"; break;
            case 2: stream << "h"; break;
            case 4: stream << "s"; break;
            case 8: stream << "d"; break;
            case 16: stream << "q"; break;
            default: stream << "?"; break;
        }

        stream << (reg - target::AArch64Register::V0);
    } else if (reg == target::AArch64Register::SP) {
        stream << "sp";
    } else {
        stream << "<invalid>";
    }
}

void AArch64AsmEmitter::emit_stack_slot(mcode::Function *func, int index) {
    mcode::StackSlot &slot = func->get_stack_frame().get_stack_slot(index);
    stream << "[sp, #" << slot.get_offset() << "]";
}

void AArch64AsmEmitter::emit_symbol(const mcode::Symbol &symbol) {
    std::string full_name = symbol_prefix + symbol.name;

    switch (symbol.reloc) {
        case mcode::Relocation::NONE: stream << full_name; break;
        case mcode::Relocation::ADR_PREL_PG_HI21: stream << full_name; break;
        case mcode::Relocation::ADD_ABS_LO12_NC: stream << ":lo12:" << full_name; break;
        case mcode::Relocation::PAGE21: stream << full_name << "@PAGE"; break;
        case mcode::Relocation::PAGEOFF12: stream << full_name << "@PAGEOFF"; break;
        case mcode::Relocation::GOT_LOAD_PAGE21: stream << full_name << "@GOTPAGE"; break;
        case mcode::Relocation::GOT_LOAD_PAGEOFF12: stream << full_name << "@GOTPAGEOFF"; break;
        default: ASSERT_UNREACHABLE;
    }
}

void AArch64AsmEmitter::emit_addr(const target::AArch64Address &addr) {
    stream << "[";

    switch (addr.get_type()) {
        case target::AArch64Address::Type::BASE:
            emit_reg(addr.get_base().get_physical_reg(), 8);
            stream << "]";
            break;
        case target::AArch64Address::Type::BASE_OFFSET_IMM:
            emit_reg(addr.get_base().get_physical_reg(), 8);
            stream << ", #" << addr.get_offset_imm() << "]";
            break;
        case target::AArch64Address::Type::BASE_OFFSET_IMM_WRITE:
            emit_reg(addr.get_base().get_physical_reg(), 8);
            stream << ", #" << addr.get_offset_imm() << "]!";
            break;
        case target::AArch64Address::Type::BASE_OFFSET_REG:
            emit_reg(addr.get_base().get_physical_reg(), 8);
            stream << ", ";
            emit_reg(addr.get_offset_reg().get_physical_reg(), 8);
            stream << "]";
            break;
        case target::AArch64Address::Type::BASE_OFFSET_SYMBOL:
            emit_reg(addr.get_base().get_physical_reg(), 8);
            stream << ", ";
            emit_symbol(addr.get_offset_symbol());
            stream << "]";
            break;
    }
}

void AArch64AsmEmitter::emit_stack_slot_offset(mcode::Function *func, mcode::Operand::StackSlotOffset offset) {
    mcode::StackFrame &frame = func->get_stack_frame();
    mcode::StackSlot &stack_slot = frame.get_stack_slot(offset.slot_index);
    int total_offset = stack_slot.get_offset() + offset.additional_offset;
    stream << "#" << total_offset;
}

void AArch64AsmEmitter::emit_condition(target::AArch64Condition condition) {
    switch (condition) {
        case target::AArch64Condition::EQ: stream << "eq"; break;
        case target::AArch64Condition::NE: stream << "ne"; break;
        case target::AArch64Condition::HS: stream << "hs"; break;
        case target::AArch64Condition::LO: stream << "lo"; break;
        case target::AArch64Condition::HI: stream << "hi"; break;
        case target::AArch64Condition::LS: stream << "ls"; break;
        case target::AArch64Condition::GE: stream << "ge"; break;
        case target::AArch64Condition::LT: stream << "lt"; break;
        case target::AArch64Condition::GT: stream << "gt"; break;
        case target::AArch64Condition::LE: stream << "le"; break;
    }
}

} // namespace codegen

} // namespace banjo
