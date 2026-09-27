#include "validator.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/primitive.hpp"

#include <set>

#define RETURN_ERROR(message)                                                                                          \
    stream << (message);                                                                                               \
    return false;

#define CHECK(condition, message)                                                                                      \
    if (!(condition)) {                                                                                                \
        RETURN_ERROR((message));                                                                                       \
    }

namespace banjo::ssa {

static bool is_value(Operand &operand) {
    return !operand.is_branch_target() && !operand.is_comparison() && !operand.is_type();
}

static bool is_type(Operand &operand) {
    return operand.is_type();
}

static bool is_addr(Operand &operand) {
    return operand.get_type() == Primitive::ADDR;
}

static bool is_int(Operand &operand) {
    // TODO: Don't allow addresses, but this requires instructions to convert
    // between address values and pointers.
    return operand.get_type().is_integer();
}

static bool is_fp(Operand &operand) {
    return operand.get_type().is_floating_point();
}

static bool is_int_imm(Operand &operand) {
    return operand.is_int_immediate();
}

Validator::Validator(std::ostream &stream) : stream{stream} {}

bool Validator::validate(Module &mod) {
    bool valid = true;

    for (Function *func : mod.get_functions()) {
        valid = valid && validate(mod, *func);
    }

    return valid;
}

bool Validator::validate(Module &mod, Function &func) {
    bool valid = true;

    std::set<VirtualRegister> defs;

    for (BasicBlock &block : func) {
        for (VirtualRegister param_reg : block.get_param_regs()) {
            defs.insert(param_reg);
        }

        for (Instruction &instr : block) {
            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }
        }
    }

    for (BasicBlock &block : func) {
        unsigned index = 0;

        for (Instruction &instr : block) {
            passes::PassUtils::iter_regs(instr.get_operands(), [&](VirtualRegister reg) {
                if (!defs.contains(reg)) {
                    stream << "error in `" << func.name << "`: %" << reg << " is not defined\n";
                    valid = false;
                }
            });

            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }

            bool instr_valid = true;

            switch (instr.get_opcode()) {
                case Opcode::ALLOCA: instr_valid = validate_alloca(instr); break;
                case Opcode::LOAD: instr_valid = validate_load(instr); break;
                case Opcode::STORE: instr_valid = validate_store(instr); break;
                case Opcode::LOADARG: instr_valid = validate_loadarg(func, instr); break;
                case Opcode::ADD: instr_valid = validate_add(instr); break;
                case Opcode::SUB: instr_valid = validate_sub(instr); break;
                case Opcode::MUL: instr_valid = validate_mul(instr); break;
                case Opcode::SDIV: instr_valid = validate_sdiv(instr); break;
                case Opcode::SREM: instr_valid = validate_srem(instr); break;
                case Opcode::UDIV: instr_valid = validate_udiv(instr); break;
                case Opcode::UREM: instr_valid = validate_urem(instr); break;
                case Opcode::FADD: instr_valid = validate_fadd(instr); break;
                case Opcode::FSUB: instr_valid = validate_fsub(instr); break;
                case Opcode::FMUL: instr_valid = validate_fmul(instr); break;
                case Opcode::FDIV: instr_valid = validate_fdiv(instr); break;
                case Opcode::AND: instr_valid = validate_and(instr); break;
                case Opcode::OR: instr_valid = validate_or(instr); break;
                case Opcode::XOR: instr_valid = validate_xor(instr); break;
                case Opcode::LSHL: instr_valid = validate_lshl(instr); break;
                case Opcode::LSHR: instr_valid = validate_lshr(instr); break;
                case Opcode::ASHR: instr_valid = validate_ashr(instr); break;
                case Opcode::JMP: instr_valid = validate_jmp(instr); break;
                case Opcode::CJMP: instr_valid = validate_cjmp(instr); break;
                case Opcode::FCJMP: instr_valid = validate_fcjmp(instr); break;
                case Opcode::SELECT: instr_valid = validate_select(instr); break;
                case Opcode::CALL: instr_valid = validate_call(instr); break;
                case Opcode::RET: instr_valid = validate_ret(instr); break;
                case Opcode::UEXTEND: instr_valid = validate_uextend(instr); break;
                case Opcode::SEXTEND: instr_valid = validate_sextend(instr); break;
                case Opcode::TRUNCATE: instr_valid = validate_truncate(instr); break;
                case Opcode::FPROMOTE: instr_valid = validate_fpromote(instr); break;
                case Opcode::FDEMOTE: instr_valid = validate_fdemote(instr); break;
                case Opcode::UTOF: instr_valid = validate_utof(instr); break;
                case Opcode::STOF: instr_valid = validate_stof(instr); break;
                case Opcode::FTOU: instr_valid = validate_ftou(instr); break;
                case Opcode::FTOS: instr_valid = validate_ftos(instr); break;
                case Opcode::BITCAST: instr_valid = validate_bitcast(instr); break;
                case Opcode::ATOMIC_LOAD: instr_valid = validate_atomic_load(instr); break;
                case Opcode::ATOMIC_STORE: instr_valid = validate_atomic_store(instr); break;
                case Opcode::ATOMIC_ADD: instr_valid = validate_atomic_add(instr); break;
                case Opcode::ATOMIC_SUB: instr_valid = validate_atomic_sub(instr); break;
                case Opcode::ATOMIC_AND: instr_valid = validate_atomic_and(instr); break;
                case Opcode::ATOMIC_OR: instr_valid = validate_atomic_or(instr); break;
                case Opcode::ATOMIC_XOR: instr_valid = validate_atomic_xor(instr); break;
                case Opcode::MEMBERPTR: instr_valid = validate_memberptr(instr); break;
                case Opcode::OFFSETPTR: instr_valid = validate_offsetptr(instr); break;
                case Opcode::COPY: instr_valid = validate_copy(instr); break;
                case Opcode::SQRT: instr_valid = validate_sqrt(instr); break;
                case Opcode::FRAME_ADDRESS: instr_valid = validate_frame_address(instr); break;
            }

            if (!instr_valid) {
                stream << " (func " << func.name << ", block " << block.get_debug_label() << ", instr " << index << ")";
                stream << '\n';
            }

            valid = valid && instr_valid;
            index += 1;
        }
    }

    return valid;
}

bool Validator::validate_alloca(Instruction &instr) {
    CHECK(instr.get_operands().size() == 1, "invalid number of operands for alloca");
    CHECK(instr.get_operand(0).is_type(), "alloca operand is not a type");
    return true;
}

bool Validator::validate_load(Instruction &instr) {
    CHECK(instr.get_operands().size() == 2, "invalid number of operands for load");
    CHECK(is_type(instr.get_operand(0)), "first operand of load is not a type");
    CHECK(is_value(instr.get_operand(1)), "second operand of load is not a value");
    CHECK(is_addr(instr.get_operand(1)), "type of load address is not `addr`");
    return true;
}

bool Validator::validate_store(Instruction &instr) {
    CHECK(instr.get_operands().size() == 2, "invalid number of operands for store");
    CHECK(is_value(instr.get_operand(0)), "first operand of store is not a value");
    CHECK(is_value(instr.get_operand(1)), "second operand of store is not a value");
    CHECK(is_addr(instr.get_operand(1)), "type of store address is not addr");
    return true;
}

bool Validator::validate_loadarg(Function &func, Instruction &instr) {
    CHECK(instr.get_operands().size() == 2, "invalid number of operands for loadarg");
    CHECK(is_type(instr.get_operand(0)), "first operand of loadarg is not a type");
    CHECK(is_int_imm(instr.get_operand(1)), "second operand of loadarg is not an integer immediate");

    LargeInt index = instr.get_operand(1).get_int_immediate();
    CHECK(index >= 0 && index < func.type.params.size(), "loadarg out of bounds");

    return true;
}

bool Validator::validate_add(Instruction &instr) {
    return validate_binary_int(instr, "add");
}

bool Validator::validate_sub(Instruction &instr) {
    return validate_binary_int(instr, "sub");
}

bool Validator::validate_mul(Instruction &instr) {
    return validate_binary_int(instr, "mul");
}

bool Validator::validate_sdiv(Instruction &instr) {
    return validate_binary_int(instr, "sdiv");
}

bool Validator::validate_srem(Instruction &instr) {
    return validate_binary_int(instr, "srem");
}

bool Validator::validate_udiv(Instruction &instr) {
    return validate_binary_int(instr, "udiv");
}

bool Validator::validate_urem(Instruction &instr) {
    return validate_binary_int(instr, "urem");
}

bool Validator::validate_fadd(Instruction &instr) {
    return validate_binary_fp(instr, "fadd");
}

bool Validator::validate_fsub(Instruction &instr) {
    return validate_binary_fp(instr, "fsub");
}

bool Validator::validate_fmul(Instruction &instr) {
    return validate_binary_fp(instr, "fmul");
}

bool Validator::validate_fdiv(Instruction &instr) {
    return validate_binary_fp(instr, "fdiv");
}

bool Validator::validate_and(Instruction &instr) {
    return validate_binary_int(instr, "and");
}

bool Validator::validate_or(Instruction &instr) {
    return validate_binary_int(instr, "or");
}

bool Validator::validate_xor(Instruction &instr) {
    return validate_binary_int(instr, "xor");
}

bool Validator::validate_lshl(Instruction &instr) {
    return validate_binary_int(instr, "lshl");
}

bool Validator::validate_lshr(Instruction &instr) {
    return validate_binary_int(instr, "lshr");
}

bool Validator::validate_ashr(Instruction &instr) {
    return validate_binary_int(instr, "ashr");
}

bool Validator::validate_jmp(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_cjmp(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_fcjmp(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_select(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_call(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_ret(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_uextend(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_sextend(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_truncate(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_fpromote(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_fdemote(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_utof(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_stof(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_ftou(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_ftos(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_bitcast(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_load(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_store(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_add(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_sub(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_and(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_or(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_atomic_xor(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_memberptr(Instruction &instr) {
    CHECK(instr.get_operands().size() == 3, "invalid number of operands for memberptr");
    CHECK(is_type(instr.get_operand(0)), "first operand of memberptr is not a type");
    CHECK(is_value(instr.get_operand(1)), "second operand of memberptr is not a value");
    CHECK(is_int_imm(instr.get_operand(2)), "third operand of memberptr is not an integer immediate");

    Type type = instr.get_operand(0).get_type();
    LargeInt index = instr.get_operand(2).get_int_immediate();

    CHECK(type.is_struct(), "memberptr into non-struct");
    Structure &struct_ = *type.get_struct();

    CHECK(!struct_.is_union, "memberptr into union struct");
    CHECK(index >= 0 && index < struct_.members.size(), "memberptr out of bounds");

    return true;
}

bool Validator::validate_offsetptr(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_copy(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_sqrt(Instruction &instr) {
    // TODO
    return true;
}

bool Validator::validate_frame_address(Instruction &instr) {
    CHECK(instr.get_operands().size() == 0, "invalid number of operands for frame_address");
    return true;
}

bool Validator::validate_binary_int(Instruction &instr, const std::string &name) {
    CHECK(instr.get_operands().size() == 2, "invalid number of operands for " + name);
    CHECK(is_value(instr.get_operand(0)), "first operand of " + name + " is not a value");
    CHECK(is_value(instr.get_operand(1)), "second operand of " + name + " is not a value");
    CHECK(instr.get_operand(0).get_type() == instr.get_operand(1).get_type(), name + " operand types don't match");
    CHECK(is_int(instr.get_operand(0)), "type of " + name + " is not an integer type");
    return true;
}

bool Validator::validate_binary_fp(Instruction &instr, const std::string &name) {
    CHECK(instr.get_operands().size() == 2, "invalid number of operands for " + name);
    CHECK(is_value(instr.get_operand(0)), "first operand of " + name + " is not a value");
    CHECK(is_value(instr.get_operand(1)), "second operand of " + name + " is not a value");
    CHECK(instr.get_operand(0).get_type() == instr.get_operand(1).get_type(), name + " operand types don't match");
    CHECK(is_fp(instr.get_operand(0)), "type of " + name + " is not a floating-point type");
    return true;
}

} // namespace banjo::ssa
