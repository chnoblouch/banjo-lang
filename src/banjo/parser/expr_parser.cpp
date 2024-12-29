#include "expr_parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/reports/report_texts.hpp"
#include "banjo/source/text_range.hpp"

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
        return type == TKN_DOT_DOT ? AST_RANGE : AST_NONE;
    });
}

ParseResult ExprParser::parse_or_level() {
    return parse_level(&ExprParser::parse_and_level, [](TokenType type) {
        return type == TKN_OR_OR ? AST_OPERATOR_OR : AST_NONE;
    });
}

ParseResult ExprParser::parse_and_level() {
    return parse_level(&ExprParser::parse_cmp_level, [](TokenType type) {
        return type == TKN_AND_AND ? AST_OPERATOR_AND : AST_NONE;
    });
}

ParseResult ExprParser::parse_cmp_level() {
    return parse_level(&ExprParser::parse_bit_or_level, [](TokenType type) {
        switch (type) {
            case TKN_EQ_EQ: return AST_OPERATOR_EQ;
            case TKN_NE: return AST_OPERATOR_NE;
            case TKN_GT: return AST_OPERATOR_GT;
            case TKN_LT: return AST_OPERATOR_LT;
            case TKN_GE: return AST_OPERATOR_GE;
            case TKN_LE: return AST_OPERATOR_LE;
            default: return AST_NONE;
        }
    });
}

ParseResult ExprParser::parse_bit_or_level() {
    return parse_level(&ExprParser::parse_bit_xor_level, [](TokenType type) {
        return type == TKN_OR ? AST_OPERATOR_BIT_OR : AST_NONE;
    });
}

ParseResult ExprParser::parse_bit_xor_level() {
    return parse_level(&ExprParser::parse_bit_and_level, [](TokenType type) {
        return type == TKN_CARET ? AST_OPERATOR_BIT_XOR : AST_NONE;
    });
}

ParseResult ExprParser::parse_bit_and_level() {
    return parse_level(&ExprParser::parse_bit_shift_level, [](TokenType type) {
        return type == TKN_AND ? AST_OPERATOR_BIT_AND : AST_NONE;
    });
}

ParseResult ExprParser::parse_bit_shift_level() {
    return parse_level(&ExprParser::parse_add_level, [](TokenType type) {
        switch (type) {
            case TKN_SHL: return AST_OPERATOR_SHL;
            case TKN_SHR: return AST_OPERATOR_SHR;
            default: return AST_NONE;
        }
    });
}

ParseResult ExprParser::parse_add_level() {
    return parse_level(&ExprParser::parse_mul_level, [](TokenType type) {
        switch (type) {
            case TKN_PLUS: return AST_OPERATOR_ADD;
            case TKN_MINUS: return AST_OPERATOR_SUB;
            default: return AST_NONE;
        }
    });
}

ParseResult ExprParser::parse_mul_level() {
    return parse_level(&ExprParser::parse_cast_level, [](TokenType type) {
        switch (type) {
            case TKN_STAR: return AST_OPERATOR_MUL;
            case TKN_SLASH: return AST_OPERATOR_DIV;
            case TKN_PERCENT: return AST_OPERATOR_MOD;
            default: return AST_NONE;
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
        stream.consume(); // Consume 'as'

        result = parser.parse_type();
        if (!result.is_valid) {
            return cast_node.build_error();
        }

        cast_node.append_child(result.node);
        return cast_node.build_with_inferred_range(AST_CAST);
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
        NodeBuilder cast_node = parser.build_node();
        cast_node.append_child(result.node);
        stream.consume(); // Consume 'as'

        result = parser.parse_type();
        if (!result.is_valid) {
            return cast_node.build_error();
        }

        cast_node.append_child(result.node);
        return cast_node.build_with_inferred_range(AST_RESULT_TYPE);
    } else {
        return result;
    }
}

ParseResult ExprParser::parse_unary_level() {
    Token *token = stream.get();

    ASTNodeType type;
    switch (token->get_type()) {
        case TKN_MINUS: type = AST_OPERATOR_NEG; break;
        case TKN_AND: type = AST_OPERATOR_REF; break;
        case TKN_STAR: type = AST_STAR_EXPR; break;
        case TKN_NOT: type = AST_OPERATOR_NOT; break;
        case TKN_QUESTION: type = AST_OPTIONAL_DATA_TYPE; break;
        default: return parse_post_operand();
    }

    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume operator
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

    if (stream.get()->is(TKN_OR_OR)) {
        stream.split_current();
    }

    switch (stream.get()->get_type()) {
        case TKN_LITERAL: return parse_number_literal();
        case TKN_CHARACTER: return parse_char_literal();
        case TKN_IDENTIFIER: return parser.create_node(AST_IDENTIFIER, stream.consume());
        case TKN_FALSE: return parser.create_node(AST_FALSE, stream.consume()->get_range());
        case TKN_TRUE: return parser.create_node(AST_TRUE, stream.consume()->get_range());
        case TKN_NULL: return parser.create_node(AST_NULL, stream.consume()->get_range());
        case TKN_NONE: return parser.create_node(AST_NONE, stream.consume()->get_range());
        case TKN_UNDEFINED: return parser.create_node(AST_UNDEFINED, stream.consume()->get_range());
        case TKN_SELF: return parser.create_node(AST_SELF, stream.consume()->get_range());
        case TKN_STRING: return parse_string_literal();
        case TKN_LBRACKET: return parse_array_literal();
        case TKN_LPAREN: return parse_paren_expr();
        case TKN_LBRACE: return parse_anon_struct_literal();
        case TKN_OR: return parse_closure();
        case TKN_I8: return parser.create_node(AST_I8, stream.consume()->get_range());
        case TKN_I16: return parser.create_node(AST_I16, stream.consume()->get_range());
        case TKN_I32: return parser.create_node(AST_I32, stream.consume()->get_range());
        case TKN_I64: return parser.create_node(AST_I64, stream.consume()->get_range());
        case TKN_U8: return parser.create_node(AST_U8, stream.consume()->get_range());
        case TKN_U16: return parser.create_node(AST_U16, stream.consume()->get_range());
        case TKN_U32: return parser.create_node(AST_U32, stream.consume()->get_range());
        case TKN_U64: return parser.create_node(AST_U64, stream.consume()->get_range());
        case TKN_F32: return parser.create_node(AST_F32, stream.consume()->get_range());
        case TKN_F64: return parser.create_node(AST_F64, stream.consume()->get_range());
        case TKN_USIZE: return parser.create_node(AST_USIZE, stream.consume()->get_range());
        case TKN_BOOL: return parser.create_node(AST_BOOL, stream.consume()->get_range());
        case TKN_ADDR: return parser.create_node(AST_ADDR, stream.consume()->get_range());
        case TKN_VOID: return parser.create_node(AST_VOID, stream.consume()->get_range());
        case TKN_DOT_DOT_DOT: return parser.create_node(AST_PARAM_SEQUENCE_TYPE, stream.consume()->get_range());
        case TKN_FUNC: return parse_func_type();
        case TKN_META: return parse_meta_expr();
        case TKN_TYPE: return parse_explicit_type();
        default: {
            parser.report_unexpected_token();
            ASTNode *node = parser.create_node(AST_ERROR, stream.previous()->get_range());
            return {node, false};
        }
    }
}

ParseResult ExprParser::parse_number_literal() {
    Token *token = stream.consume();
    std::string str = token->get_value();

    ASTNode *node;

    if (str.find('.') != std::string::npos) {
        return parser.create_node(AST_FLOAT_LITERAL, token);
    } else {
        node = parser.create_node(AST_INT_LITERAL, token);
    }

    node->set_range(token->get_range());
    return node;
}

ParseResult ExprParser::parse_char_literal() {
    Token *token = stream.consume();
    std::string value = token->get_value().substr(1, token->get_value().size() - 2);
    return parser.create_node(AST_CHAR_LITERAL, value, token->get_range());
}

ParseResult ExprParser::parse_string_literal() {
    Token *token = stream.consume();
    std::string value = token->get_value().substr(1, token->get_value().size() - 2);
    return parser.create_node(AST_STRING_LITERAL, value, token->get_range());
}

ParseResult ExprParser::parse_array_literal() {
    ASTNodeType type = AST_ARRAY_EXPR;

    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume '['

    while (true) {
        if (stream.get()->is(TKN_RBRACKET)) {
            stream.consume();
            break;
        } else {
            ParseResult result = ExprParser(parser, true).parse();
            if (!result.is_valid) {
                return result;
            }

            if (stream.get()->is(TKN_COLON)) {
                type = AST_MAP_EXPR;
                stream.consume(); // Consume ':'

                NodeBuilder pair_node = parser.build_node();
                pair_node.append_child(result.node);
                result = ExprParser(parser, true).parse();
                pair_node.append_child(result.node);

                node.append_child(pair_node.build(AST_MAP_EXPR_PAIR));
            } else if (stream.get()->is(TKN_SEMI)) {
                type = AST_STATIC_ARRAY_TYPE;
                stream.consume(); // Consume ';'

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
            stream.consume();
            break;
        } else if (stream.get()->is(TKN_COMMA)) {
            stream.consume();
        } else {
            parser.report_unexpected_token();
            return {node.build(type), false};
        }
    }

    ASTNode *result = node.build(type);
    if (parser.mode == Parser::Mode::FORMATTING && stream.peek(-2)->is(TKN_COMMA)) {
        result->flags.trailing_comma = true;
    }

    return result;
}

ParseResult ExprParser::parse_paren_expr() {
    TextPosition start = stream.get()->get_position();
    stream.consume(); // Consume opening '('

    in_parentheses = true;
    ASTNode *sub_expression = ExprParser(parser, true).parse().node;
    in_parentheses = false;

    if (stream.get()->is(TKN_COMMA)) {
        ASTNode *tuple_literal = parser.create_node(AST_TUPLE_EXPR);
        tuple_literal->append_child(sub_expression);

        while (stream.get()->is(TKN_COMMA)) {
            stream.consume(); // Consume ','

            if (stream.get()->is(TKN_RPAREN)) {
                break;
            }

            tuple_literal->append_child(ExprParser(parser, true).parse().node);
        }

        tuple_literal->set_range({start, stream.get()->get_end()});
        stream.consume(); // Consume ')'

        if (parser.mode == Parser::Mode::FORMATTING && stream.peek(-2)->is(TKN_COMMA)) {
            tuple_literal->flags.trailing_comma = true;
        }

        return tuple_literal;
    }

    sub_expression->set_range({start, stream.get()->get_end()});
    stream.consume(); // Consume ')'

    if (parser.mode == Parser::Mode::FORMATTING) {
        ASTNode *paren_expr = parser.create_node(AST_PAREN_EXPR, sub_expression->get_range());
        paren_expr->append_child(sub_expression);
        return paren_expr;
    } else {
        return sub_expression;
    }
}

ParseResult ExprParser::parse_anon_struct_literal() {
    NodeBuilder node = parser.build_node();

    if (!allow_struct_literals) {
        parser.report_unexpected_token();
        return node.build_error();
    }

    ParseResult result = parse_struct_literal_body();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(AST_ANON_STRUCT_LITERAL);
}

ParseResult ExprParser::parse_closure() {
    NodeBuilder node = parser.build_node();

    ParseResult result = parser.parse_param_list(TKN_OR);
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    if (stream.get()->is(TKN_ARROW)) {
        stream.consume(); // Consume '->'
        node.append_child(parser.parse_type().node);
    } else {
        node.append_child(parser.create_node(AST_VOID));
    }

    if (!stream.get()->is(TKN_LBRACE) || type_expected) {
        return node.build(AST_CLOSURE_TYPE);
    }

    result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(AST_CLOSURE);
}

ParseResult ExprParser::parse_func_type() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'func'

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
        stream.consume(); // Consume '->'
        node.append_child(parser.parse_type().node);
    } else {
        node.append_child(parser.create_node(AST_VOID));
    }

    return node.build(AST_FUNCTION_DATA_TYPE);
}

ParseResult ExprParser::parse_meta_expr() {
    NodeBuilder builder = parser.build_node();
    stream.consume(); // Consume 'meta'

    stream.consume(); // Consume '('
    ParseResult result = parse();
    stream.consume(); // Consume ')'

    builder.append_child(result.node);
    return {builder.build(AST_META_EXPR), result.is_valid};
}

ParseResult ExprParser::parse_explicit_type() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'type'

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "(");
        return node.build_error();
    }
    stream.consume(); // Consume '('

    ParseResult result = parser.parse_type();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    if (!stream.get()->is(TKN_RPAREN)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, ")");
        return node.build_error();
    }
    stream.consume(); // Consume ')'

    return node.build(AST_EXPLICIT_TYPE);
}

ParseResult ExprParser::parse_dot_expr(ASTNode *lhs_node) {
    bool is_meta_field_access = lhs_node->get_type() == AST_EXPLICIT_TYPE;
    ASTNode *operator_node =
        is_meta_field_access ? parser.create_node(AST_META_FIELD_ACCESS) : parser.create_node(AST_DOT_OPERATOR);

    operator_node->append_child(lhs_node);
    stream.consume(); // Consume '.'
    ParseResult result = parse_operand();
    operator_node->append_child(result.node);

    operator_node->set_range_from_children();
    return {operator_node, result.is_valid};
}

ParseResult ExprParser::parse_call_expr(ASTNode *lhs_node) {
    bool is_meta_method_call = lhs_node->get_type() == AST_META_FIELD_ACCESS;
    ASTNode *node =
        is_meta_method_call ? parser.create_node(AST_META_METHOD_CALL) : parser.create_node(AST_FUNCTION_CALL);
    node->append_child(lhs_node);

    ParseResult result = parser.parse_list(AST_FUNCTION_ARGUMENT_LIST, TKN_RPAREN, [this](NodeBuilder &) {
        return ExprParser(parser, true).parse();
    });
    node->append_child(result.node);

    node->set_range_from_children();
    return {node, result.is_valid};
}

ParseResult ExprParser::parse_bracket_expr(ASTNode *lhs_node) {
    NodeBuilder node = parser.build_node();
    node.set_start_position(lhs_node->get_range().start);
    node.append_child(lhs_node);

    ParseResult result = parser.parse_list(AST_FUNCTION_ARGUMENT_LIST, TKN_RBRACKET, [this](NodeBuilder &) {
        return ExprParser(parser).parse();
    });
    node.append_child(result.node);

    return {node.build(AST_ARRAY_ACCESS), result.is_valid};
}

ParseResult ExprParser::parse_struct_literal(ASTNode *lhs_node) {
    ASTNode *node = parser.create_node(AST_STRUCT_INSTANTIATION);
    node->append_child(lhs_node);

    ParseResult result = parse_struct_literal_body();
    node->append_child(result.node);
    node->set_range_from_children();

    if (!result.is_valid) {
        node->set_type(AST_ERROR);
    }

    return {node, result.is_valid};
}

ParseResult ExprParser::parse_struct_literal_body() {
    return parser.parse_list(AST_STRUCT_FIELD_VALUE_LIST, TKN_RBRACE, [this](NodeBuilder &) -> ParseResult {
        if (parser.is_at_completion_point()) {
            return {parser.parse_completion_point(), true};
        }

        ASTNode *value_node = parser.create_node(AST_STRUCT_FIELD_VALUE);
        ASTNode *name_node;

        if (stream.get()->is(TKN_IDENTIFIER)) {
            name_node = parser.create_node(AST_IDENTIFIER, stream.consume());
            value_node->append_child(name_node);
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
            return {value_node, false};
        }

        if (stream.get()->is(TKN_COLON)) {
            stream.consume(); // Consume ':'
            value_node->append_child(parse().node);
        } else if (stream.get()->is(TKN_COMMA) || stream.get()->is(TKN_RBRACE)) {
            value_node->append_child(parser.create_node(AST_IDENTIFIER, name_node->get_value(), name_node->get_range())
            );
        } else {
            parser.report_unexpected_token();
            return {value_node, false};
        }

        value_node->set_range_from_children();
        return value_node;
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
    ASTNodeType type = token_checker(stream.get()->get_type());

    while (type != AST_NONE) {
        Token *token = stream.consume();
        ASTNode *operator_node = parser.create_node(type, token->get_range());
        operator_node->append_child(current_node);

        result = (this->*child_builder)();
        operator_node->append_child(result.node);
        operator_node->set_range_from_children();
        current_node = operator_node;

        if (!result.is_valid) {
            return {current_node, false};
        }

        type = token_checker(stream.get()->get_type());
    }

    return current_node;
}

} // namespace lang

} // namespace banjo
