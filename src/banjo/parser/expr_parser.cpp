#include "expr_parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/reports/report_texts.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

ExprParser::ExprParser(Parser &parser, bool allow_struct_literals /* = false */)
  : parser(parser),
    stream(parser.stream),
    allow_struct_literals(allow_struct_literals) {}

ParseResult ExprParser::parse() {
    return parse_range_level();
}

ParseResult ExprParser::parse_type() {
    type_expected = true;
    return parse_result_level();
}

ParseResult ExprParser::parse_range_level() {
    return parse_level(&ExprParser::parse_or_level, [](TokenType type) {
        return type == TKN_DOT_DOT ? AST_RANGE_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_or_level() {
    return parse_level(&ExprParser::parse_and_level, [](TokenType type) {
        return type == TKN_OR_OR ? AST_OR_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_and_level() {
    return parse_level(&ExprParser::parse_cmp_level, [](TokenType type) {
        return type == TKN_AND_AND ? AST_AND_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_cmp_level() {
    return parse_level(&ExprParser::parse_bit_or_level, [](TokenType type) {
        switch (type) {
            case TKN_EQ_EQ: return AST_EQ_EXPR;
            case TKN_NE: return AST_NE_EXPR;
            case TKN_GT: return AST_GT_EXPR;
            case TKN_LT: return AST_LT_EXPR;
            case TKN_GE: return AST_GE_EXPR;
            case TKN_LE: return AST_LE_EXPR;
            default: return AST_NONE_LITERAL;
        }
    });
}

ParseResult ExprParser::parse_bit_or_level() {
    return parse_level(&ExprParser::parse_bit_xor_level, [](TokenType type) {
        return type == TKN_OR ? AST_BIT_OR_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_bit_xor_level() {
    return parse_level(&ExprParser::parse_bit_and_level, [](TokenType type) {
        return type == TKN_CARET ? AST_BIT_XOR_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_bit_and_level() {
    return parse_level(&ExprParser::parse_bit_shift_level, [](TokenType type) {
        return type == TKN_AND ? AST_BIT_AND_EXPR : AST_NONE_LITERAL;
    });
}

ParseResult ExprParser::parse_bit_shift_level() {
    return parse_level(&ExprParser::parse_add_level, [](TokenType type) {
        switch (type) {
            case TKN_SHL: return AST_SHL_EXPR;
            case TKN_SHR: return AST_SHR_EXPR;
            default: return AST_NONE_LITERAL;
        }
    });
}

ParseResult ExprParser::parse_add_level() {
    return parse_level(&ExprParser::parse_mul_level, [](TokenType type) {
        switch (type) {
            case TKN_PLUS: return AST_ADD_EXPR;
            case TKN_MINUS: return AST_SUB_EXPR;
            default: return AST_NONE_LITERAL;
        }
    });
}

ParseResult ExprParser::parse_mul_level() {
    return parse_level(&ExprParser::parse_cast_level, [](TokenType type) {
        switch (type) {
            case TKN_STAR: return AST_MUL_EXPR;
            case TKN_SLASH: return AST_DIV_EXPR;
            case TKN_PERCENT: return AST_MOD_EXPR;
            default: return AST_NONE_LITERAL;
        }
    });
}

ParseResult ExprParser::parse_cast_level() {
    ParseResult result = parse_result_level();
    if (!result.is_valid) {
        return result;
    }

    if (stream.get()->is(TKN_AS)) {
        NodeBuilder cast_node = parser.build_node();
        cast_node.append_child(result.node);
        cast_node.consume(); // Consume 'as'

        result = parser.parse_type();
        if (!result.is_valid) {
            return cast_node.build_error();
        }

        cast_node.append_child(result.node);
        return cast_node.build_with_inferred_range(AST_CAST_EXPR);
    } else {
        return result;
    }
}

ParseResult ExprParser::parse_result_level() {
    ParseResult result = parse_unary_level();
    if (!result.is_valid) {
        return result;
    }

    if (stream.get()->is(TKN_EXCEPT)) {
        NodeBuilder node = parser.build_node();
        node.append_child(result.node);
        node.consume(); // Consume 'except'

        result = parser.parse_type();
        if (!result.is_valid) {
            return node.build_error();
        }

        node.append_child(result.node);
        return node.build_with_inferred_range(AST_RESULT_TYPE);
    } else {
        return result;
    }
}

ParseResult ExprParser::parse_unary_level() {
    Token *token = stream.get();

    ASTNodeType type;
    switch (token->type) {
        case TKN_MINUS: type = AST_NEG_EXPR; break;
        case TKN_TILDE: type = AST_BIT_NOT_EXPR; break;
        case TKN_AND: type = AST_ADDR_EXPR; break;
        case TKN_STAR: type = AST_STAR_EXPR; break;
        case TKN_NOT: type = AST_NOT_EXPR; break;
        case TKN_QUESTION: type = AST_OPTIONAL_TYPE; break;
        case TKN_DOT: type = AST_IMPLICIT_DOT_EXPR; break;
        case TKN_REF: type = AST_REF_EXPR; break;
        case TKN_TRY: type = AST_TRY_EXPR; break;
        default: return parse_post_operand();
    }

    NodeBuilder node = parser.build_node();
    node.consume(); // Consume operator

    if (type == AST_REF_EXPR && stream.get()->is(TKN_MUT)) {
        node.consume();
        type = AST_REF_MUT_EXPR;
    }

    ParseResult result = parse_unary_level();
    node.append_child(result.node);
    return {node.build(type), result.is_valid};
}

ParseResult ExprParser::parse_post_operand() {
    ParseResult result = parse_operand();
    ASTNode *current_node = result.node;

    if (!result.is_valid) {
        return result;
    }

    while (true) {
        ParseResult result;

        if (stream.get()->is(TKN_DOT)) result = parse_dot_expr(current_node);
        else if (stream.get()->is(TKN_LPAREN)) result = parse_call_expr(current_node);
        else if (stream.get()->is(TKN_LBRACKET)) result = parse_bracket_expr(current_node);
        else if (stream.get()->is(TKN_LBRACE) && allow_struct_literals) result = parse_struct_literal(current_node);
        else break;

        current_node = result.node;

        if (!result.is_valid) {
            return {current_node, false};
        }
    }

    return current_node;
}

ParseResult ExprParser::parse_operand() {
    if (parser.is_at_completion_point()) {
        return parser.parse_completion_point();
    }

    switch (stream.get()->type) {
        case TKN_LITERAL: return parse_number_literal();
        case TKN_CHARACTER: return parse_char_literal();
        case TKN_IDENTIFIER: return parser.consume_into_node(AST_IDENTIFIER);
        case TKN_FALSE: return parser.consume_into_node(AST_FALSE_LITERAL);
        case TKN_TRUE: return parser.consume_into_node(AST_TRUE_LITERAL);
        case TKN_NULL: return parser.consume_into_node(AST_NULL_LITERAL);
        case TKN_NONE: return parser.consume_into_node(AST_NONE_LITERAL);
        case TKN_UNDEFINED: return parser.consume_into_node(AST_UNDEFINED_LITERAL);
        case TKN_SELF: return parser.consume_into_node(AST_SELF);
        case TKN_STRING: return parse_string_literal();
        case TKN_LBRACKET: return parse_array_literal();
        case TKN_LPAREN: return parse_paren_expr();
        case TKN_LBRACE: return parse_anon_struct_literal();
        case TKN_OR: return parse_closure();
        case TKN_OR_OR: return parse_closure();
        case TKN_I8: return parser.consume_into_node(AST_I8);
        case TKN_I16: return parser.consume_into_node(AST_I16);
        case TKN_I32: return parser.consume_into_node(AST_I32);
        case TKN_I64: return parser.consume_into_node(AST_I64);
        case TKN_U8: return parser.consume_into_node(AST_U8);
        case TKN_U16: return parser.consume_into_node(AST_U16);
        case TKN_U32: return parser.consume_into_node(AST_U32);
        case TKN_U64: return parser.consume_into_node(AST_U64);
        case TKN_F32: return parser.consume_into_node(AST_F32);
        case TKN_F64: return parser.consume_into_node(AST_F64);
        case TKN_USIZE: return parser.consume_into_node(AST_USIZE);
        case TKN_BOOL: return parser.consume_into_node(AST_BOOL);
        case TKN_ADDR: return parser.consume_into_node(AST_ADDR);
        case TKN_VOID: return parser.consume_into_node(AST_VOID);
        case TKN_DOT_DOT_DOT: return parser.consume_into_node(AST_PARAM_SEQUENCE_TYPE);
        case TKN_FUNC: return parse_func_type();
        case TKN_META: return parse_meta_expr();
        default: {
            parser.report_unexpected_token();
            ASTNode *node = parser.create_node(AST_ERROR, stream.previous()->range());
            return {node, false};
        }
    }
}

ParseResult ExprParser::parse_number_literal() {
    if (stream.get()->value.find('.') != std::string::npos) {
        return parser.consume_into_node(AST_FP_LITERAL);
    } else {
        return parser.consume_into_node(AST_INT_LITERAL);
    }
}

ParseResult ExprParser::parse_char_literal() {
    Token *token = stream.consume();
    std::string_view value{token->value.substr(1, token->value.size() - 2)};

    ASTNode *node = parser.create_node(AST_CHAR_LITERAL, value, token->range());
    node->tokens = parser.mod->create_token_index(stream.get_position() - 1);
    return node;
}

ParseResult ExprParser::parse_string_literal() {
    Token *token = stream.consume();
    std::string_view value{token->value.substr(1, token->value.size() - 2)};

    ASTNode *node = parser.create_node(AST_STRING_LITERAL, value, token->range());
    node->tokens = parser.mod->create_token_index(stream.get_position() - 1);
    return node;
}

ParseResult ExprParser::parse_array_literal() {
    ASTNodeType type = AST_ARRAY_LITERAL;

    NodeBuilder node = parser.build_node();
    node.consume(); // Consume '['

    while (true) {
        if (stream.get()->is(TKN_RBRACKET)) {
            node.consume();
            break;
        } else {
            ParseResult result = ExprParser(parser, true).parse();
            if (!result.is_valid) {
                return result;
            }

            if (stream.get()->is(TKN_COLON)) {
                type = AST_MAP_LITERAL;

                NodeBuilder pair_node = parser.build_node();
                pair_node.append_child(result.node);
                pair_node.consume(); // Consume ':'
                result = ExprParser(parser, true).parse();
                pair_node.append_child(result.node);

                node.append_child(pair_node.build(AST_MAP_LITERAL_ENTRY));
            } else if (stream.get()->is(TKN_SEMI)) {
                type = AST_STATIC_ARRAY_TYPE;
                node.consume(); // Consume ';'

                node.append_child(result.node);

                ParseResult result = ExprParser(*this).parse();
                if (result.is_valid) {
                    node.append_child(result.node);
                } else {
                    return node.build_error();
                }
            } else {
                node.append_child(result.node);
            }
        }

        if (stream.get()->is(TKN_RBRACKET)) {
            node.consume();
            break;
        } else if (stream.get()->is(TKN_COMMA)) {
            node.consume();
        } else {
            parser.report_unexpected_token();
            return {node.build(type), false};
        }
    }

    return node.build(type);
}

ParseResult ExprParser::parse_paren_expr() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume '('

    ASTNode *sub_expression = ExprParser(parser, true).parse().node;

    if (stream.get()->is(TKN_COMMA)) {
        node.append_child(sub_expression);

        while (stream.get()->is(TKN_COMMA)) {
            node.consume(); // Consume ','

            if (stream.get()->is(TKN_RPAREN)) {
                break;
            }

            node.append_child(ExprParser(parser, true).parse().node);
        }

        node.consume(); // Consume ')'
        return node.build(AST_TUPLE_EXPR);
    }

    node.consume(); // Consume ')'
    node.append_child(sub_expression);

    return node.build(AST_PAREN_EXPR);
}

ParseResult ExprParser::parse_anon_struct_literal() {
    NodeBuilder node = parser.build_node();

    if (!allow_struct_literals) {
        parser.report_unexpected_token();
        return node.build_error();
    }

    ParseResult result = parse_struct_literal_body();
    if (!result.is_valid) {
        return node.build_error(AST_TYPELESS_STRUCT_LITERAL);
    }
    node.append_child(result.node);

    return node.build(AST_TYPELESS_STRUCT_LITERAL);
}

ParseResult ExprParser::parse_closure() {
    NodeBuilder node = parser.build_node();

    if (stream.get()->is(TKN_OR_OR)) {
        node.append_child(parser.consume_into_node(AST_PARAM_LIST));
    } else if (stream.get()->is(TKN_OR)) {
        ParseResult result = parser.parse_param_list(TKN_OR);
        if (!result.is_valid) {
            return node.build_error();
        }

        node.append_child(result.node);
    } else {
        ASSERT_UNREACHABLE;
    }

    if (stream.get()->is(TKN_ARROW)) {
        node.consume(); // Consume '->'
        node.append_child(parser.parse_return_type().node);
    } else {
        node.append_child(parser.create_node(AST_EMPTY));
    }

    if (!stream.get()->is(TKN_LBRACE) || type_expected) {
        return node.build(AST_CLOSURE_TYPE);
    }

    ParseResult result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(AST_CLOSURE_LITERAL);
}

ParseResult ExprParser::parse_func_type() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'func'

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token();
        return node.build_error();
    }

    ParseResult result = parser.parse_param_list();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    if (stream.get()->is(TKN_ARROW)) {
        node.consume(); // Consume '->'
        node.append_child(parser.parse_return_type().node);
    } else {
        node.append_child(parser.create_node(AST_EMPTY));
    }

    return node.build(AST_FUNC_TYPE);
}

ParseResult ExprParser::parse_meta_expr() {
    NodeBuilder builder = parser.build_node();
    builder.consume(); // Consume 'meta'

    builder.consume(); // Consume '('
    ParseResult result = parse();
    builder.consume(); // Consume ')'

    builder.append_child(result.node);
    return {builder.build(AST_META_ACCESS), result.is_valid};
}

ParseResult ExprParser::parse_dot_expr(ASTNode *lhs_node) {
    NodeBuilder operator_node = parser.build_node();
    operator_node.set_start_position(lhs_node->range.start);

    operator_node.append_child(lhs_node);
    operator_node.consume(); // Consume '.'
    ParseResult result = parse_operand();
    operator_node.append_child(result.node);

    return {operator_node.build(AST_DOT_EXPR), result.is_valid};
}

ParseResult ExprParser::parse_call_expr(ASTNode *lhs_node) {
    ASTNode *node = parser.create_node(AST_CALL_EXPR);
    node->append_child(lhs_node);

    ParseResult result =
        parser.parse_list(AST_ARG_LIST, TKN_RPAREN, [this]() { return ExprParser(parser, true).parse(); });
    node->append_child(result.node);

    node->set_range_from_children();
    return {node, result.is_valid};
}

ParseResult ExprParser::parse_bracket_expr(ASTNode *lhs_node) {
    NodeBuilder node = parser.build_node();
    node.set_start_position(lhs_node->range.start);
    node.append_child(lhs_node);

    ParseResult result = parser.parse_list(AST_ARG_LIST, TKN_RBRACKET, [this]() { return ExprParser(parser).parse(); });
    node.append_child(result.node);

    return {node.build(AST_BRACKET_EXPR), result.is_valid};
}

ParseResult ExprParser::parse_struct_literal(ASTNode *lhs_node) {
    NodeBuilder node = parser.build_node();
    node.set_start_position(lhs_node->range.start);
    node.append_child(lhs_node);

    ParseResult result = parse_struct_literal_body();
    node.append_child(result.node);

    if (result.is_valid) {
        return node.build(AST_STRUCT_LITERAL);
    } else {
        return node.build_error(AST_STRUCT_LITERAL);
    }
}

ParseResult ExprParser::parse_struct_literal_body() {
    return parser.parse_list(AST_STRUCT_LITERAL_ENTRY_LIST, TKN_RBRACE, [this]() -> ParseResult {
        if (parser.is_at_completion_point()) {
            return {parser.parse_completion_point(), true};
        }

        NodeBuilder entry_node = parser.build_node();
        ASTNode *name_node;

        if (stream.get()->is(TKN_IDENTIFIER)) {
            name_node = parser.consume_into_node(AST_IDENTIFIER);
            entry_node.append_child(name_node);
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
            return entry_node.build_error();
        }

        if (stream.get()->is(TKN_COLON)) {
            entry_node.consume(); // Consume ':'
            entry_node.append_child(parse().node);
        }

        return entry_node.build(AST_STRUCT_LITERAL_ENTRY);
    });
}

ParseResult ExprParser::parse_level(
    ParseResult (ExprParser::*child_builder)(),
    ASTNodeType(token_checker)(TokenType type)
) {
    ParseResult result = (this->*child_builder)();
    if (!result.is_valid) {
        return result;
    }

    ASTNode *current_node = result.node;
    ASTNodeType type = token_checker(stream.get()->type);

    while (type != AST_NONE_LITERAL) {
        Token *token = stream.consume();
        unsigned token_index = stream.get_position() - 1;

        ASTNode *operator_node = parser.create_node(type, token->range());
        operator_node->append_child(current_node);

        result = (this->*child_builder)();
        operator_node->append_child(result.node);
        operator_node->set_range_from_children();
        operator_node->tokens = parser.mod->create_token_index(token_index);
        current_node = operator_node;

        if (!result.is_valid) {
            return {current_node, false};
        }

        type = token_checker(stream.get()->type);
    }

    return current_node;
}

} // namespace lang

} // namespace banjo
