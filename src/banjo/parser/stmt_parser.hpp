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

    ParseResult parse_assign(ASTNode *lhs_node, ASTNodeType type);
    ParseResult parse_var();
    ParseResult parse_if_chain();
    ParseResult parse_switch();
    ParseResult parse_try();
    ParseResult parse_while();
    ParseResult parse_for();
    ParseResult parse_break();
    ParseResult parse_continue();
    ParseResult parse_return();
    ParseResult parse_meta_stmt();

private:
    ParseResult parse_var_with_type(NodeBuilder &node);
    ParseResult parse_var_without_type(NodeBuilder &node);
    ParseResult parse_meta_if(NodeBuilder &node);
    ParseResult parse_meta_for(NodeBuilder &node);
};

} // namespace lang

#endif
