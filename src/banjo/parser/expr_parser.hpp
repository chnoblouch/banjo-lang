#ifndef EXPR_PARSER_H
#define EXPR_PARSER_H

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "lexer/token.hpp"
#include "parser/node_builder.hpp"
#include "parser/parser.hpp"

namespace banjo {

namespace lang {

class ExprParser {

private:
    Parser &parser;
    TokenStream &stream;
    bool allow_struct_literals;

    bool type_expected = false;
    bool in_parentheses = false;

public:
    ExprParser(Parser &parser, bool allow_struct_literals = false);
    ParseResult parse();
    ParseResult parse_type();

private:
    ParseResult parse_range_level();
    ParseResult parse_or_level();
    ParseResult parse_and_level();
    ParseResult parse_cmp_level();
    ParseResult parse_bit_or_level();
    ParseResult parse_bit_xor_level();
    ParseResult parse_bit_and_level();
    ParseResult parse_bit_shift_level();
    ParseResult parse_add_level();
    ParseResult parse_mul_level();
    ParseResult parse_cast_level();
    ParseResult parse_result_level();
    ParseResult parse_unary_level();
    ParseResult parse_post_operand();
    ParseResult parse_operand();

    ParseResult parse_number_literal();
    ParseResult parse_char_literal();
    ParseResult parse_string_literal();
    ParseResult parse_array_literal();
    ParseResult parse_paren_expr();
    ParseResult parse_anon_struct_literal();
    ParseResult parse_closure();
    ParseResult parse_func_type();
    ParseResult parse_meta_expr();
    ParseResult parse_explicit_type();

    ParseResult parse_dot_expr(ASTNode *lhs_node);
    ParseResult parse_call_expr(ASTNode *lhs_node);
    ParseResult parse_bracket_expr(ASTNode *lhs_node);
    ParseResult parse_struct_literal(ASTNode *lhs_node);

    ParseResult parse_struct_literal_body();
    ParseResult parse_level(ParseResult (ExprParser::*child_builder)(), ASTNodeType(token_checker)(TokenType type));
};

} // namespace lang

} // namespace banjo

#endif
