#include "ssa_parser.hpp"

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/macros.hpp"

#include "line_based_reader.hpp"

#include <iostream>
#include <string_view>
#include <unordered_map>

namespace banjo {
namespace test {

const std::unordered_map<std::string_view, ssa::Primitive> PRIMITIVES = {
    {"void", ssa::Primitive::VOID},
    {"i8", ssa::Primitive::I8},
    {"i16", ssa::Primitive::I16},
    {"i32", ssa::Primitive::I32},
    {"i64", ssa::Primitive::I64},
    {"f32", ssa::Primitive::F32},
    {"f64", ssa::Primitive::F64},
    {"addr", ssa::Primitive::ADDR},
};

const std::unordered_map<std::string_view, ssa::Opcode> OPS = {
    {"alloca", ssa::Opcode::ALLOCA},
    {"load", ssa::Opcode::LOAD},
    {"store", ssa::Opcode::STORE},
    {"loadarg", ssa::Opcode::LOADARG},
    {"add", ssa::Opcode::ADD},
    {"sub", ssa::Opcode::SUB},
    {"mul", ssa::Opcode::MUL},
    {"sdiv", ssa::Opcode::SDIV},
    {"srem", ssa::Opcode::SREM},
    {"udiv", ssa::Opcode::UDIV},
    {"urem", ssa::Opcode::UREM},
    {"fadd", ssa::Opcode::FADD},
    {"fsub", ssa::Opcode::FSUB},
    {"fmul", ssa::Opcode::FMUL},
    {"fdiv", ssa::Opcode::FDIV},
    {"and", ssa::Opcode::AND},
    {"or", ssa::Opcode::OR},
    {"xor", ssa::Opcode::XOR},
    {"shl", ssa::Opcode::SHL},
    {"shr", ssa::Opcode::SHR},
    {"jmp", ssa::Opcode::JMP},
    {"cjmp", ssa::Opcode::CJMP},
    {"fcjmp", ssa::Opcode::FCJMP},
    {"select", ssa::Opcode::SELECT},
    {"call", ssa::Opcode::CALL},
    {"ret", ssa::Opcode::RET},
    {"uextend", ssa::Opcode::UEXTEND},
    {"sextend", ssa::Opcode::SEXTEND},
    {"fpromote", ssa::Opcode::FPROMOTE},
    {"truncate", ssa::Opcode::TRUNCATE},
    {"fdemote", ssa::Opcode::FDEMOTE},
    {"utof", ssa::Opcode::UTOF},
    {"stof", ssa::Opcode::STOF},
    {"ftou", ssa::Opcode::FTOU},
    {"ftos", ssa::Opcode::FTOS},
    {"memberptr", ssa::Opcode::MEMBERPTR},
    {"offsetptr", ssa::Opcode::OFFSETPTR},
    {"copy", ssa::Opcode::COPY},
    {"sqrt", ssa::Opcode::SQRT},
};

const std::unordered_map<std::string_view, ssa::Comparison> COMPARISONS = {
    {"eq", ssa::Comparison::EQ},
    {"ne", ssa::Comparison::NE},
    {"ugt", ssa::Comparison::UGT},
    {"uge", ssa::Comparison::UGE},
    {"ult", ssa::Comparison::ULT},
    {"ule", ssa::Comparison::ULE},
    {"sgt", ssa::Comparison::SGT},
    {"sge", ssa::Comparison::SGE},
    {"slt", ssa::Comparison::SLT},
    {"sle", ssa::Comparison::SLE},
    {"feq", ssa::Comparison::FEQ},
    {"fne", ssa::Comparison::FNE},
    {"fgt", ssa::Comparison::FGT},
    {"fge", ssa::Comparison::FGE},
    {"flt", ssa::Comparison::FLT},
    {"fle", ssa::Comparison::FLE},
};

const std::unordered_map<std::string_view, ssa::CallingConv> CALLING_CONVS = {
    {"sysv_abi", ssa::CallingConv::X86_64_SYS_V_ABI},
    {"ms_abi", ssa::CallingConv::X86_64_MS_ABI},
    {"aapcs", ssa::CallingConv::AARCH64_AAPCS},
};

SSAParser::SSAParser() : reader(std::cin) {}

ssa::Module SSAParser::parse() {
    while (reader.next_line()) {
        reader.skip_whitespace();
        if (reader.get() == '#') {
            continue;
        }

        std::string_view start = reader.read_until_whitespace();

        if (start == "func") {
            ssa::CallingConv calling_conv = parse_calling_conv();
            ssa::Type return_type = parse_type();
            std::string name = parse_identifier();
            std::vector<ssa::Type> params = parse_params();
            reader.skip_whitespace();

            ssa::FunctionType type{
                .params = params,
                .return_type = return_type,
                .calling_conv = calling_conv,
                .variadic = false,
                .first_variadic_index = 0,
            };

            if (reader.get() == ':') {
                ssa::Function *func = new ssa::Function(name, type);
                func->set_next_reg(10000);
                funcs.insert({func->name, func});
                mod.add(func);
            } else if (reader.get() == '\0') {
                ssa::FunctionDecl *func_decl = new ssa::FunctionDecl(name, type);
                mod.add(func_decl);
            } else {
                ASSERT_UNREACHABLE;
            }
        } else if (start == "struct") {
            std::string name = parse_identifier();
            ssa::Structure *struct_ = new ssa::Structure(name);
            mod.add(struct_);
        } else if (start.starts_with("@")) {
            ssa::Function *cur_func = mod.get_functions().back();

            std::string label = std::string(start).substr(1, start.size() - 2);
            cur_func->basic_blocks.append(ssa::BasicBlock(label));

            ssa::BasicBlockIter iter = cur_func->basic_blocks.get_last_iter();
            blocks.insert({iter->get_label(), iter});
        }
    }

    reader.reset();

    ssa::Function *cur_func = nullptr;
    ssa::BasicBlockIter cur_block = nullptr;
    ssa::Structure *cur_struct = nullptr;

    while (reader.next_line()) {
        reader.skip_whitespace();
        std::string_view start = reader.read_until_whitespace();

        if (start == "func") {
            parse_calling_conv();
            parse_type();
            std::string name = parse_identifier();

            auto iter = funcs.find(name);
            if (iter != funcs.end()) {
                cur_func = iter->second;
                cur_block = cur_func->get_entry_block_iter();
                cur_struct = nullptr;
            }
        } else if (start == "struct") {
            std::string name = parse_identifier();

            cur_func = nullptr;
            cur_block = nullptr;
            cur_struct = mod.get_structure(name);
        } else if (start.starts_with("@")) {
            std::string label = std::string(start).substr(1, start.size() - 2);
            cur_block = blocks.at(label);
        } else if (start.starts_with("%")) {
            unsigned reg_number = std::stoul(std::string(start).substr(1));
            ssa::VirtualRegister reg = static_cast<ssa::VirtualRegister>(reg_number);

            reader.skip_whitespace();
            reader.skip_char('=');
            reader.skip_whitespace();

            cur_block->append(parse_instr(reg));
        } else if (start == "field") {
            ssa::Type type = parse_type();
            std::string name = parse_identifier();
            cur_struct->add(ssa::StructureMember{.name = name, .type = type});
        } else if (OPS.contains(start)) {
            reader.restart_line();
            cur_block->append(parse_instr({}));
        }
    }

    return std::move(mod);
}

std::string SSAParser::parse_identifier() {
    reader.skip_whitespace();
    reader.consume();

    std::string identifier;

    while (LineBasedReader::is_alphanumeric(reader.get()) || reader.get() == '_') {
        identifier += reader.consume();
    }

    return identifier;
}

ssa::Type SSAParser::parse_type() {
    reader.skip_whitespace();

    std::string string;

    while (LineBasedReader::is_alphanumeric(reader.get()) || reader.get() == '@') {
        string += reader.consume();
    }

    ssa::Type base_type;

    if (string[0] == '@') {
        base_type = mod.get_structure(string.substr(1));
    } else {
        base_type = PRIMITIVES.at(string);
    }

    reader.skip_whitespace();

    if (reader.get() == '[') {
        reader.consume();

        std::string string;

        while (LineBasedReader::is_numeric(reader.get())) {
            string += reader.consume();
        }

        reader.skip_char(']');

        unsigned length = std::stoul(string);

        ssa::Type type = base_type;
        type.set_array_length(length);
        return type;
    } else {
        return base_type;
    }
}

std::vector<ssa::Type> SSAParser::parse_params() {
    reader.skip_whitespace();
    reader.skip_char('(');
    reader.skip_whitespace();

    std::vector<ssa::Type> params;

    while (reader.get() != ')') {
        params.push_back(parse_type());
        reader.skip_whitespace();

        if (reader.get() == ',') {
            reader.skip_char(',');
            reader.skip_whitespace();
        } else {
            ASSERT(reader.get() == ')');
        }
    }

    reader.consume();
    return params;
}

ssa::CallingConv SSAParser::parse_calling_conv() {
    reader.skip_whitespace();

    std::string string;

    while (LineBasedReader::is_alpha(reader.get()) || reader.get() == '_') {
        string += reader.consume();
    }

    return CALLING_CONVS.at(string);
}

ssa::Instruction SSAParser::parse_instr(std::optional<ssa::VirtualRegister> dst) {
    ssa::Opcode op = parse_op();
    std::vector<ssa::Operand> operands = parse_operands();
    return {op, dst, operands};
}

ssa::Opcode SSAParser::parse_op() {
    reader.skip_whitespace();

    std::string_view string = reader.read_until_whitespace();
    return OPS.at(string);
}

std::vector<ssa::Operand> SSAParser::parse_operands() {
    reader.skip_whitespace();

    std::vector<ssa::Operand> operands;

    while (reader.get() != '\0' && reader.get() != '!') {
        if (std::optional<ssa::Operand> operand = parse_operand()) {
            operands.push_back(*operand);
        } else {
            break;
        }

        if (reader.get() == ',') {
            reader.consume();
        }
    }

    return operands;
}

std::optional<ssa::Operand> SSAParser::parse_operand() {
    reader.skip_whitespace();

    if (reader.get() == '!') {
        return {};
    }

    ssa::Type type = ssa::Primitive::VOID;

    if (LineBasedReader::is_alpha(reader.get()) || reader.get() == '@') {
        type = parse_type();
    }

    reader.skip_whitespace();

    if (reader.get() == ',' || reader.get() == '\0' || reader.get() == '!') {
        return ssa::Operand::from_type(type);
    }

    std::string string;

    while (reader.get() != ',' && reader.get() != '\0' && reader.get() != '!' &&
           !LineBasedReader::is_whitespace(reader.get())) {
        string += reader.consume();
    }

    if (string[0] == '-' || LineBasedReader::is_numeric(string[0])) {
        if (string.find('.') == std::string::npos) {
            return ssa::Operand::from_int_immediate(LargeInt{string}, type);
        } else {
            return ssa::Operand::from_fp_immediate(std::stod(string), type);
        }
    } else if (string[0] == '%') {
        unsigned reg_number = std::stoul(string.substr(1));
        ssa::VirtualRegister reg = static_cast<ssa::VirtualRegister>(reg_number);
        return ssa::Operand::from_register(reg, type);
    } else if (string[0] == '@') {
        std::string name = string.substr(1);

        ssa::Function *func = mod.get_function(name);
        if (func) {
            return ssa::Operand::from_func(func, type);
        }

        ssa::FunctionDecl *external_func = mod.get_external_function(name);
        if (external_func) {
            return ssa::Operand::from_extern_func(external_func, type);
        }

        auto block_iter = blocks.find(name);
        if (block_iter != blocks.end()) {
            ssa::BranchTarget target{
                .block = block_iter->second,
                .args{},
            };

            return ssa::Operand::from_branch_target(target, type);
        }

        std::cout << name << "\n";
        
        return {};
    } else if (COMPARISONS.contains(string)) {
        return ssa::Operand::from_comparison(COMPARISONS.at(string), type);
    } else {
        return {};
    }
}

} // namespace test
} // namespace banjo
