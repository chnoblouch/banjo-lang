#ifndef DECL_PARSER_H
#define DECL_PARSER_H

#include "parser/parser.hpp"
#include <unordered_set>

namespace lang {

class DeclParser {

private:
    static const std::unordered_set<TokenType> RECOVERY_TOKENS;

    Parser &parser;
    TokenStream &stream;

public:
    DeclParser(Parser &parser);
    ASTNode *parse();

private:
    ParseResult parse_func(ASTNode *qualifier_list);
    ParseResult parse_const();
    ParseResult parse_struct();
    ParseResult parse_enum();
    ParseResult parse_union();
    ParseResult parse_union_case();
    ParseResult parse_type_alias();

    ParseResult parse_qualifiers();

    ParseResult parse_native();
    ParseResult parse_native_var();
    ParseResult parse_native_func();

    bool parse_func_head(NodeBuilder &node, TokenType terminator, bool &generic);

    void recover();
};

} // namespace lang

#endif
