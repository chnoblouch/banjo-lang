#ifndef DECL_PARSER_H
#define DECL_PARSER_H

#include "parser/parser.hpp"

namespace lang {

class DeclParser {

private:
    Parser &parser;
    TokenStream &stream;

public:
    DeclParser(Parser &parser);

    ParseResult parse_func(ASTNode *qualifier_list);
    ParseResult parse_const();
    ParseResult parse_struct();
    ParseResult parse_enum();
    ParseResult parse_union();
    ParseResult parse_union_case();
    ParseResult parse_proto();
    ParseResult parse_type_alias();
    ParseResult parse_use();
    ParseResult parse_pub();
    ParseResult parse_native();

private:
    ParseResult parse_use_tree();
    ParseResult parse_use_tree_element();

    ParseResult parse_qualifiers();

    ParseResult parse_native_var();
    ParseResult parse_native_func();

    bool parse_func_head(NodeBuilder &node, bool &generic);
    ParseResult parse_generic_param_list();
};

} // namespace lang

#endif
