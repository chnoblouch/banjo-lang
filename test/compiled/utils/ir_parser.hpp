#ifndef TEST_IR_PARSER_H
#define TEST_IR_PARSER_H

#include "banjo/ssa/module.hpp"
#include <istream>

namespace banjo {

class IRParser {

private:
    std::istream &stream;
    std::string line;
    unsigned pos;

    ssa::Module mod;
    ssa::Function *cur_func;
    ssa::Structure *cur_struct;
    ssa::BasicBlockIter cur_block;

public:
    IRParser(std::istream &stream);
    ssa::Module parse();

private:
    void enter_func();
    void parse_instr();
    void enter_struct();
    void parse_field();

    ssa::VirtualRegister parse_reg();
    ssa::Opcode parse_opcode();
    ssa::Operand parse_operand();
    ssa::Type parse_type();
    std::string parse_ident();

    char get();
    char consume();
    std::string read_until(char c);
    void skip_whitespace();
    bool end_of_line();
};

} // namespace banjo

#endif
