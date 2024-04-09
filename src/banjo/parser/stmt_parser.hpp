#ifndef STMT_PARSER_H
#define STMT_PARSER_H

#include "parser/parser.hpp"

namespace lang {

class StmtParser {

private:
    Parser &parser;
    TokenStream &stream;

public:
    StmtParser(Parser &parser);
    ASTNode *parse();

private:
    ParseResult parse_var();
    ParseResult parse_var_with_type(NodeBuilder &node);
    ParseResult parse_var_without_type(NodeBuilder &node);
    ParseResult parse_if_chain();
    ParseResult parse_switch();
    ParseResult parse_while();
    ParseResult parse_for();
    ParseResult parse_return();
    ParseResult parse_meta_stmt();
    ParseResult parse_meta_if(NodeBuilder &node);
    ParseResult parse_meta_for(NodeBuilder &node);
};

} // namespace lang

#endif
