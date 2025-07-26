#ifndef BANJO_PARSER_STMT_PARSER_H
#define BANJO_PARSER_STMT_PARSER_H

#include "banjo/parser/parser.hpp"

namespace banjo {

namespace lang {

class StmtParser {

private:
    Parser &parser;
    TokenStream &stream;

public:
    StmtParser(Parser &parser);

    ParseResult parse_assign(ASTNode *lhs_node, ASTNodeType type);
    ParseResult parse_var();
    ParseResult parse_ref();
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
    ParseResult parse_var_or_ref(TextPosition start, ASTNodeType type, ASTNodeType type_typeless);
    ParseResult parse_var_with_type(NodeBuilder &node, ASTNodeType type);
    ParseResult parse_var_without_type(NodeBuilder &node, ASTNodeType type);
    ParseResult parse_meta_if(NodeBuilder &node);
    ParseResult parse_meta_for(NodeBuilder &node);
};

} // namespace lang

} // namespace banjo

#endif
