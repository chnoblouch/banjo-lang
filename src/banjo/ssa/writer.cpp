#include "writer.hpp"

namespace banjo {

namespace ssa {

Writer::Writer(std::ostream &stream) : stream(stream) {}

void Writer::write(Module &mod) {
    if (!mod.get_structures().empty()) {
        for (ssa::Structure *struct_ : mod.get_structures()) {
            stream << "struct @" << struct_->name << " {\n";
            for (const StructureMember &member : struct_->members) {
                stream << "    " << type_to_str(member.type) << " @" << member.name << "\n";
            }
            stream << "}\n\n";
        }
    }

    if (!mod.get_external_functions().empty()) {
        for (FunctionDecl &external_function : mod.get_external_functions()) {
            write_func_decl(external_function);
            stream << "\n";
        }
        stream << "\n";
    }

    if (!mod.get_external_globals().empty()) {
        for (GlobalDecl &external_global : mod.get_external_globals()) {
            stream << "decl global " << type_to_str(external_global.get_type()) << " @" << external_global.get_name()
                   << "\n";
        }
        stream << "\n";
    }

    if (!mod.get_globals().empty()) {
        for (Global &global : mod.get_globals()) {
            stream << "def global " << type_to_str(global.get_type()) << " @" << global.get_name();
            stream << " = " << (global.initial_value ? value_to_str(*global.initial_value) : "undefined") << "\n";
        }
        stream << "\n";
    }

    if (!mod.get_dll_exports().empty()) {
        for (const std::string &dll_export : mod.get_dll_exports()) {
            stream << "def dllexport @" << dll_export << "\n";
        }
        stream << "\n";
    }

    if (!mod.get_functions().empty()) {
        for (Function *function : mod.get_functions()) {
            write_func_def(function);
            stream << "\n";
        }
    }

    stream.flush();
}

void Writer::write_func_decl(FunctionDecl &func_decl) {
    stream << "decl func ";
    stream << calling_conv_to_str(func_decl.get_calling_conv()) + " ";
    stream << type_to_str(func_decl.get_return_type()) + " ";
    stream << "@" + func_decl.get_name();
    stream << "(";

    for (int i = 0; i < func_decl.get_params().size(); i++) {
        Type param = func_decl.get_params()[i];
        stream << type_to_str(param);

        if (i != func_decl.get_params().size() - 1) {
            stream << ", ";
        }
    }

    stream << ")";
}

void Writer::write_func_def(Function *func_def) {
    stream << "def func ";
    stream << calling_conv_to_str(func_def->get_calling_conv()) + " ";
    stream << type_to_str(func_def->get_return_type()) + " ";
    stream << "@" + func_def->get_name();
    stream << "(";

    for (int i = 0; i < func_def->get_params().size(); i++) {
        Type param = func_def->get_params()[i];
        stream << type_to_str(param);

        if (i != func_def->get_params().size() - 1) {
            stream << ", ";
        }
    }

    stream << ")" << "\n";

    for (BasicBlock &basic_block : func_def->get_basic_blocks()) {
        write_basic_block(basic_block);
    }
}

void Writer::write_basic_block(BasicBlock &basic_block) {
    if (basic_block.has_label()) {
        stream << basic_block.get_label();

        if (!basic_block.get_param_regs().empty()) {
            stream << "(";

            for (unsigned i = 0; i < basic_block.get_param_regs().size(); i++) {
                stream << type_to_str(basic_block.get_param_types()[i]);
                stream << " %" << basic_block.get_param_regs()[i];

                if (i != basic_block.get_param_regs().size() - 1) {
                    stream << ", ";
                }
            }

            stream << ")";
        }

        stream << ":\n";
    }

    for (ssa::Instruction &instr : basic_block) {
        stream << "    ";

        if (instr.get_dest()) {
            stream << reg_to_str(*instr.get_dest()) << " = ";
        }

        std::string opcode;
        switch (instr.get_opcode()) {
            case Opcode::INVALID: opcode = "invalid"; break;
            case Opcode::ALLOCA: opcode = "alloca"; break;
            case Opcode::LOAD: opcode = "load"; break;
            case Opcode::STORE: opcode = "store"; break;
            case Opcode::LOADARG: opcode = "loadarg"; break;
            case Opcode::ADD: opcode = "add"; break;
            case Opcode::SUB: opcode = "sub"; break;
            case Opcode::MUL: opcode = "mul"; break;
            case Opcode::SDIV: opcode = "sdiv"; break;
            case Opcode::SREM: opcode = "srem"; break;
            case Opcode::UDIV: opcode = "udiv"; break;
            case Opcode::UREM: opcode = "urem"; break;
            case Opcode::AND: opcode = "and"; break;
            case Opcode::OR: opcode = "or"; break;
            case Opcode::XOR: opcode = "xor"; break;
            case Opcode::SHL: opcode = "shl"; break;
            case Opcode::SHR: opcode = "shr"; break;
            case Opcode::FADD: opcode = "fadd"; break;
            case Opcode::FSUB: opcode = "fsub"; break;
            case Opcode::FMUL: opcode = "fmul"; break;
            case Opcode::FDIV: opcode = "fdiv"; break;
            case Opcode::JMP: opcode = "jmp"; break;
            case Opcode::CJMP: opcode = "cjmp"; break;
            case Opcode::FCJMP: opcode = "fcjmp"; break;
            case Opcode::SELECT: opcode = "select"; break;
            case Opcode::CALL: opcode = "call"; break;
            case Opcode::CALLINTR: opcode = "callintr"; break;
            case Opcode::RET: opcode = "ret"; break;
            case Opcode::UEXTEND: opcode = "uextend"; break;
            case Opcode::SEXTEND: opcode = "sextend"; break;
            case Opcode::TRUNCATE: opcode = "truncate"; break;
            case Opcode::FPROMOTE: opcode = "fpromote"; break;
            case Opcode::FDEMOTE: opcode = "fdemote"; break;
            case Opcode::UTOF: opcode = "utof"; break;
            case Opcode::STOF: opcode = "stof"; break;
            case Opcode::FTOU: opcode = "ftou"; break;
            case Opcode::FTOS: opcode = "ftos"; break;
            case Opcode::MEMBERPTR: opcode = "memberptr"; break;
            case Opcode::OFFSETPTR: opcode = "offsetptr"; break;
            case Opcode::COPY: opcode = "copy"; break;
            case Opcode::SQRT: opcode = "sqrt"; break;
            case Opcode::ASM: opcode = "asm"; break;
            default: opcode = "???"; break;
        }

        stream << opcode;

        for (int i = 0; i < instr.get_operands().size(); i++) {
            if (i == 0) {
                stream << " ";
            }

            Operand &operand = instr.get_operands()[i];

            if (operand.get_type() != Type(Primitive::VOID)) {
                stream << type_to_str(operand.get_type());
                if (!operand.is_type()) {
                    stream << " ";
                }
            }

            stream.flush();
            stream << value_to_str(operand);

            if (i != instr.get_operands().size() - 1) {
                stream << ", ";
            }
        }

        if (instr.get_flags() & ssa::Instruction::FLAG_ARG_STORE) stream << " !arg_store";
        if (instr.get_flags() & ssa::Instruction::FLAG_SAVE_ARG) stream << " !save_arg";

        stream << "\n";
    }

    stream << "\n";
}

std::string Writer::reg_to_str(VirtualRegister reg) {
    return "%" + std::to_string(reg);
}

std::string Writer::type_to_str(Type type) {
    std::string str;

    if (type.is_primitive()) {
        switch (type.get_primitive()) {
            case Primitive::VOID: str = "void"; break;
            case Primitive::I8: str = "i8"; break;
            case Primitive::I16: str = "i16"; break;
            case Primitive::I32: str = "i32"; break;
            case Primitive::I64: str = "i64"; break;
            case Primitive::F32: str = "f32"; break;
            case Primitive::F64: str = "f64"; break;
            case Primitive::ADDR: str = "addr"; break;
        }
    } else if (type.is_struct()) {
        str = "@" + type.get_struct()->name;
    }

    if (type.get_array_length() != 1) {
        str += "[" + std::to_string(type.get_array_length()) + "]";
    }

    return str;
}

std::string Writer::value_to_str(Value value) {
    if (value.is_int_immediate()) return value.get_int_immediate().to_string();
    else if (value.is_fp_immediate()) return std::to_string(value.get_fp_immediate());
    else if (value.is_register()) return reg_to_str(value.get_register());
    else if (value.is_symbol()) return "@" + value.get_symbol_name();
    else if (value.is_string()) {
        std::string str = "\"";
        for (char c : value.get_string()) {
            if (c == '\0') str += "\\0";
            else if (c == '\n') str += "\\n";
            else if (c == '\"') str += "\\\"";
            else if (c == '\\') str += "\\\\";
            else str += c;
        }
        str += "\"";
        return str;
    } else if (value.is_branch_target()) {
        std::string str = "@" + value.get_branch_target().block->get_label();

        if (!value.get_branch_target().args.empty()) {
            str += "(";

            for (unsigned i = 0; i < value.get_branch_target().args.size(); i++) {
                str += value_to_str(value.get_branch_target().args[i]);

                if (i != value.get_branch_target().args.size() - 1) {
                    str += ", ";
                }
            }

            str += ")";
        }

        return str;
    } else if (value.is_comparison()) {
        switch (value.get_comparison()) {
            case Comparison::EQ: return "eq";
            case Comparison::NE: return "ne";
            case Comparison::UGT: return "ugt";
            case Comparison::UGE: return "uge";
            case Comparison::ULT: return "ult";
            case Comparison::ULE: return "ule";
            case Comparison::SGT: return "sgt";
            case Comparison::SGE: return "sge";
            case Comparison::SLT: return "slt";
            case Comparison::SLE: return "sle";
            case Comparison::FEQ: return "feq";
            case Comparison::FNE: return "fne";
            case Comparison::FGT: return "fgt";
            case Comparison::FGE: return "fge";
            case Comparison::FLT: return "flt";
            case Comparison::FLE: return "fle";
            default: return "???";
        }
    } else if (value.is_type()) return "";
    else return "???";
}

std::string Writer::calling_conv_to_str(CallingConv calling_conv) {
    switch (calling_conv) {
        case CallingConv::X86_64_SYS_V_ABI: return "sysv_abi";
        case CallingConv::X86_64_MS_ABI: return "ms_abi";
        case CallingConv::AARCH64_AAPCS: return "aapcs";
        default: return "???";
    }
}

} // namespace ssa

} // namespace banjo
