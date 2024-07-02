#ifndef TEST_IR_PARSER_H
#define TEST_IR_PARSER_H

#include "banjo/ir/module.hpp"
#include <istream>

namespace banjo {

class IRParser {

private:
    std::istream &stream;
    std::string line;
    unsigned pos;

    ir::Module mod;
    ir::Function *cur_func;
    ir::Structure *cur_struct;
    ir::BasicBlockIter cur_block;

public:
    IRParser(std::istream &stream);
    ir::Module parse();

private:
    void enter_func();
    void parse_instr();
    void enter_struct();
    void parse_field();

    ir::VirtualRegister parse_reg();
    ir::Opcode parse_opcode();
    ir::Operand parse_operand();
    ir::Type parse_type();
    std::string parse_ident();

    char get();
    char consume();
    std::string read_until(char c);
    void skip_whitespace();
    bool end_of_line();
};

} // namespace banjo

#endif
