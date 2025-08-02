#include "ir_parser.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/utils/macros.hpp"

#include <cctype>
#include <iostream>

namespace banjo {

IRParser::IRParser(std::istream &stream) : stream(stream) {}

ssa::Module IRParser::parse() {
    while (std::getline(stream, line)) {
        pos = 0;

        skip_whitespace();
        if (end_of_line()) {
            continue;
        }

        if (line.starts_with("func")) {
            enter_func();
        } else if (line.starts_with("struct")) {
            enter_struct();
        } else if (cur_func) {
            parse_instr();
        } else if (cur_struct) {
            parse_field();
        }
    }

    return std::move(mod);
}

void IRParser::enter_func() {
    cur_struct = nullptr;

    cur_func = new ssa::Function(
        "none",
        ssa::FunctionType{
            .params = {},
            .return_type = ssa::Primitive::VOID,
            .calling_conv = ssa::CallingConv::X86_64_MS_ABI,
        }
    );

    cur_block = cur_func->get_entry_block_iter();
    mod.add(cur_func);

    for (unsigned i = 0; i < 10000; i++) {
        cur_func->next_virtual_reg();
    }
}

void IRParser::parse_instr() {
    ssa::Instruction instr;

    if (get() == '%') {
        instr.set_dest(parse_reg());
        skip_whitespace();
        consume(); // '='
        skip_whitespace();
    }

    instr.set_opcode(parse_opcode());
    skip_whitespace();

    if (!end_of_line()) {
        instr.get_operands().push_back(parse_operand());
        skip_whitespace();

        while (get() == ',') {
            consume(); // ','
            skip_whitespace();
            instr.get_operands().push_back(parse_operand());
        }
    }

    cur_block->append(instr);
}

void IRParser::enter_struct() {
    cur_func = nullptr;

    // 'struct'
    while (std::isalnum(get())) {
        consume();
    }
    skip_whitespace();

    std::string name = parse_ident();
    mod.add(new ssa::Structure(name));
    cur_struct = mod.get_structure(name);
}

void IRParser::parse_field() {
    ssa::Type type = parse_type();
    skip_whitespace();
    std::string name = parse_ident();
    cur_struct->add(ssa::StructureMember{name, type});
}

ssa::VirtualRegister IRParser::parse_reg() {
    consume(); // '%'

    std::string str;
    while (std::isdigit(get())) {
        str += consume();
    }

    return std::stoi(str);
}

ssa::Opcode IRParser::parse_opcode() {
    std::string str;
    while (isalpha(get()) || get() == '_') {
        str += consume();
    }

    if (str == "alloca") return ssa::Opcode::ALLOCA;
    else if (str == "load") return ssa::Opcode::LOAD;
    else if (str == "store") return ssa::Opcode::STORE;
    else if (str == "loadarg") return ssa::Opcode::LOADARG;
    else if (str == "add") return ssa::Opcode::ADD;
    else if (str == "sub") return ssa::Opcode::SUB;
    else if (str == "mul") return ssa::Opcode::MUL;
    else if (str == "sdiv") return ssa::Opcode::SDIV;
    else if (str == "srem") return ssa::Opcode::SREM;
    else if (str == "udiv") return ssa::Opcode::UDIV;
    else if (str == "urem") return ssa::Opcode::UREM;
    else if (str == "fadd") return ssa::Opcode::FADD;
    else if (str == "fsub") return ssa::Opcode::FSUB;
    else if (str == "fmul") return ssa::Opcode::FMUL;
    else if (str == "fdiv") return ssa::Opcode::FDIV;
    else if (str == "and") return ssa::Opcode::AND;
    else if (str == "or") return ssa::Opcode::OR;
    else if (str == "xor") return ssa::Opcode::XOR;
    else if (str == "shl") return ssa::Opcode::SHL;
    else if (str == "shr") return ssa::Opcode::SHR;
    else if (str == "jmp") return ssa::Opcode::JMP;
    else if (str == "cjmp") return ssa::Opcode::CJMP;
    else if (str == "fcjmp") return ssa::Opcode::FCJMP;
    else if (str == "select") return ssa::Opcode::SELECT;
    else if (str == "call") return ssa::Opcode::CALL;
    else if (str == "ret") return ssa::Opcode::RET;
    else if (str == "uextend") return ssa::Opcode::UEXTEND;
    else if (str == "sextend") return ssa::Opcode::SEXTEND;
    else if (str == "fpromote") return ssa::Opcode::FPROMOTE;
    else if (str == "truncate") return ssa::Opcode::TRUNCATE;
    else if (str == "fdemote") return ssa::Opcode::FDEMOTE;
    else if (str == "utof") return ssa::Opcode::UTOF;
    else if (str == "stof") return ssa::Opcode::STOF;
    else if (str == "ftou") return ssa::Opcode::FTOU;
    else if (str == "ftos") return ssa::Opcode::FTOS;
    else if (str == "memberptr") return ssa::Opcode::MEMBERPTR;
    else if (str == "offsetptr") return ssa::Opcode::OFFSETPTR;
    else if (str == "copy") return ssa::Opcode::COPY;
    else if (str == "sqrt") return ssa::Opcode::SQRT;
    else ASSERT_UNREACHABLE;
}

ssa::Operand IRParser::parse_operand() {
    ssa::Type type = parse_type();
    skip_whitespace();

    if (get() == '%') {
        return ssa::Operand::from_register(parse_reg(), type);
    } else if (get() == '@') {
        ASSERT_UNREACHABLE;
        // return ssa::Operand::from_extern_func(parse_ident(), type);
    } else if (get() == '-' || isdigit(get())) {
        std::string str;
        while (get() == '-' || get() == '.' || isdigit(get())) {
            str += consume();
        }

        if (str.find('.') == std::string::npos) {
            return ssa::Operand::from_int_immediate(LargeInt{str}, type);
        } else {
            return ssa::Operand::from_fp_immediate(std::stod(str));
        }
    } else {
        return ssa::Operand::from_type(type);
    }
}

ssa::Type IRParser::parse_type() {
    if (get() == '@') {
        return ssa::Type(mod.get_structure(parse_ident()));
    } else {
        std::string str;
        while (std::isalnum(get())) {
            str += consume();
        }

        if (str == "void") return ssa::Primitive::VOID;
        else if (str == "i8") return ssa::Primitive::I8;
        else if (str == "i16") return ssa::Primitive::I16;
        else if (str == "i32") return ssa::Primitive::I32;
        else if (str == "i64") return ssa::Primitive::I64;
        else if (str == "f32") return ssa::Primitive::F32;
        else if (str == "f64") return ssa::Primitive::F64;
        else if (str == "addr") return ssa::Primitive::ADDR;
        else return ssa::Primitive::VOID;
    }
}

std::string IRParser::parse_ident() {
    consume(); // '@'

    std::string str;
    while (std::isalnum(get()) || get() == '_') {
        str += consume();
    }

    return str;
}

char IRParser::get() {
    return end_of_line() ? '\0' : line[pos];
}

char IRParser::consume() {
    return line[pos++];
}

void IRParser::skip_whitespace() {
    char c = line[pos];
    while (!end_of_line() && (c == ' ' || c == '\t')) {
        c = line[++pos];
    }
}

bool IRParser::end_of_line() {
    return pos == line.size();
}

} // namespace banjo
