#include "ir_parser.hpp"

#include "ir/primitive.hpp"
#include "utils/macros.hpp"

#include <cctype>
#include <iostream>

IRParser::IRParser(std::istream &stream) : stream(stream) {}

ir::Module IRParser::parse() {
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

    cur_func = new ir::Function("none", {}, ir::Primitive::VOID, ir::CallingConv::X86_64_MS_ABI);
    cur_block = cur_func->get_entry_block_iter();
    mod.add(cur_func);
    
    for (unsigned i = 0; i < 10000; i++) {
        cur_func->next_virtual_reg();
    }
}

void IRParser::parse_instr() {
    ir::Instruction instr;

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
    while(std::isalnum(get())) {
        consume();
    }
    skip_whitespace();

    std::string name = parse_ident();
    mod.add(new ir::Structure(name));
    cur_struct = mod.get_structure(name);
}

void IRParser::parse_field() {
    ir::Type type = parse_type();
    skip_whitespace();
    std::string name = parse_ident();
    cur_struct->add(ir::StructureMember{name, type});
}

ir::VirtualRegister IRParser::parse_reg() {
    consume(); // '%'

    std::string str;
    while (std::isdigit(get())) {
        str += consume();
    }

    return std::stoi(str);
}

ir::Opcode IRParser::parse_opcode() {
    std::string str;
    while (isalpha(get()) || get() == '_') {
        str += consume();
    }

    if (str == "alloca") return ir::Opcode::ALLOCA;
    else if (str == "load") return ir::Opcode::LOAD;
    else if (str == "store") return ir::Opcode::STORE;
    else if (str == "loadarg") return ir::Opcode::LOADARG;
    else if (str == "add") return ir::Opcode::ADD;
    else if (str == "sub") return ir::Opcode::SUB;
    else if (str == "mul") return ir::Opcode::MUL;
    else if (str == "sdiv") return ir::Opcode::SDIV;
    else if (str == "srem") return ir::Opcode::SREM;
    else if (str == "udiv") return ir::Opcode::UDIV;
    else if (str == "urem") return ir::Opcode::UREM;
    else if (str == "fadd") return ir::Opcode::FADD;
    else if (str == "fsub") return ir::Opcode::FSUB;
    else if (str == "fmul") return ir::Opcode::FMUL;
    else if (str == "fdiv") return ir::Opcode::FDIV;
    else if (str == "and") return ir::Opcode::AND;
    else if (str == "or") return ir::Opcode::OR;
    else if (str == "xor") return ir::Opcode::XOR;
    else if (str == "shl") return ir::Opcode::SHL;
    else if (str == "shr") return ir::Opcode::SHR;
    else if (str == "jmp") return ir::Opcode::JMP;
    else if (str == "cjmp") return ir::Opcode::CJMP;
    else if (str == "fcjmp") return ir::Opcode::FCJMP;
    else if (str == "select") return ir::Opcode::SELECT;
    else if (str == "call") return ir::Opcode::CALL;
    else if (str == "ret") return ir::Opcode::RET;
    else if (str == "uextend") return ir::Opcode::UEXTEND;
    else if (str == "sextend") return ir::Opcode::SEXTEND;
    else if (str == "fpromote") return ir::Opcode::FPROMOTE;
    else if (str == "truncate") return ir::Opcode::TRUNCATE;
    else if (str == "fdemote") return ir::Opcode::FDEMOTE;
    else if (str == "utof") return ir::Opcode::UTOF;
    else if (str == "stof") return ir::Opcode::STOF;
    else if (str == "ftou") return ir::Opcode::FTOU;
    else if (str == "ftos") return ir::Opcode::FTOS;
    else if (str == "memberptr") return ir::Opcode::MEMBERPTR;
    else if (str == "offsetptr") return ir::Opcode::OFFSETPTR;
    else if (str == "copy") return ir::Opcode::COPY;
    else if (str == "sqrt") return ir::Opcode::SQRT;
    else if (str == "asm") return ir::Opcode::ASM;
    else return ir::Opcode::INVALID;
}

ir::Operand IRParser::parse_operand() {
    ir::Type type = parse_type();
    skip_whitespace();

    if (get() == '%') {
        return ir::Operand::from_register(parse_reg(), type);
    } else if (get() == '@') {
        return ir::Operand::from_extern_func(parse_ident(), type);  
    } else if (get() == '-' || isdigit(get())) {
        std::string str;
        while (get() == '-' || get() == '.' || isdigit(get())) {
            str += consume();
        }

        if (str.find('.') == std::string::npos) {
            return ir::Operand::from_int_immediate(LargeInt{str}, type);
        } else {
            return ir::Operand::from_fp_immediate(std::stod(str));
        }
    } else {
        return ir::Operand::from_type(type);
    }
}

ir::Type IRParser::parse_type() {
    if (get() == '@') {
        return ir::Type(mod.get_structure(parse_ident()));
    } else {
        std::string str;
        while (std::isalnum(get())) {
            str += consume();
        }

        if (str == "void") return ir::Primitive::VOID;
        else if (str == "i8") return ir::Primitive::I8;
        else if (str == "i16") return ir::Primitive::I16;
        else if (str == "i32") return ir::Primitive::I32;
        else if (str == "i64") return ir::Primitive::I64;
        else if (str == "f32") return ir::Primitive::F32;
        else if (str == "f64") return ir::Primitive::F64;
        else if (str == "addr") return ir::Primitive::ADDR;
        else return ir::Primitive::VOID;
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