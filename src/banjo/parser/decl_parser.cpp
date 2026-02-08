#include "decl_parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/expr_parser.hpp"
#include "banjo/parser/node_builder.hpp"

namespace banjo {

namespace lang {

DeclParser::DeclParser(Parser &parser) : parser(parser), stream(parser.stream) {}

ParseResult DeclParser::parse_func(ASTNode *qualifier_list) {
    NodeBuilder node = parser.build_node();

    if (qualifier_list) {
        node.append_child(qualifier_list);
    } else {
        node.append_child(parser.create_node(AST_QUALIFIER_LIST));
    }

    node.consume(); // Consume 'func'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    bool generic;
    bool head_valid = parse_func_head(node, generic);

    if (!head_valid) {
        node.append_child(parser.create_dummy_block());
        ASTNodeType type = generic ? AST_GENERIC_FUNC_DEF : AST_FUNC_DEF;
        return {node.build(type), false};
    }

    if (stream.get()->is(TKN_SEMI)) {
        node.consume(); // Consume ';'
        return {node.build(AST_FUNC_DECL), true};
    } else if (!stream.get()->is(TKN_LBRACE)) {
        if (stream.previous()->end_of_line) {
            return {node.build(AST_FUNC_DECL), true};
        }

        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'{'");
        node.append_child(parser.create_dummy_block());

        ASTNodeType type = generic ? AST_GENERIC_FUNC_DEF : AST_FUNC_DEF;
        return {node.build(type), false};
    }

    ParseResult result = parser.parse_block();
    node.append_child(result.node);

    ASTNodeType type = generic ? AST_GENERIC_FUNC_DEF : AST_FUNC_DEF;
    return {node.build(type), result.is_valid};
}

ParseResult DeclParser::parse_const() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'const'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        node.append_child(parser.create_node(AST_ERROR));
        node.append_child(parser.create_node(AST_ERROR));
        node.append_child(parser.create_node(AST_ERROR));
        return node.build(AST_CONST_DEF);
    }

    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    if (!stream.get()->is(TKN_COLON)) {
        parser.report_unexpected_token();
        node.append_child(parser.create_node(AST_ERROR));
        node.append_child(parser.create_node(AST_ERROR));
        return node.build(AST_CONST_DEF);
    }

    node.consume(); // Consume ':'
    node.append_child(parser.parse_type().node);

    if (!stream.get()->is(TKN_EQ)) {
        parser.report_unexpected_token();
        node.append_child(parser.create_node(AST_ERROR));
        return node.build(AST_CONST_DEF);
    }

    node.consume(); // Consume '='
    node.append_child(parser.parse_expression());

    return parser.check_stmt_terminator(node, AST_CONST_DEF);
}

ParseResult DeclParser::parse_struct() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'struct'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    if (stream.get()->is(TKN_LBRACKET)) {
        ParseResult result = parse_generic_param_list();
        if (!result.is_valid) {
            return node.build_error();
        }

        node.append_child(result.node);

        result = parser.parse_block();
        node.append_child(result.node);

        if (!result.is_valid) {
            return {node.build(AST_GENERIC_STRUCT_DEF), false};
        }
        return node.build(AST_GENERIC_STRUCT_DEF);
    }

    if (stream.get()->is(TKN_COLON)) {
        // TODO: Don't accept empty lists.
        ParseResult result =
            parser.parse_list(AST_IMPL_LIST, TKN_LBRACE, [this]() { return ExprParser(parser).parse_type(); }, false);

        if (result.is_valid) {
            node.append_child(result.node);
        } else {
            return node.build_error();
        }
    } else {
        node.append_child(parser.create_node(AST_IMPL_LIST));
    }

    ParseResult result = parser.parse_block();
    node.append_child(result.node);

    if (!result.is_valid) {
        return {node.build(AST_STRUCT_DEF), false};
    }
    return node.build(AST_STRUCT_DEF);
}

ParseResult DeclParser::parse_enum() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'enum'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    ParseResult result = parser.parse_list(AST_ENUM_VARIANT_LIST, TKN_RBRACE, [this]() {
        NodeBuilder variant = parser.build_node();
        variant.append_child(parser.consume_into_node(AST_IDENTIFIER));

        if (stream.get()->is(TKN_EQ)) {
            variant.consume(); // Consume '='
            variant.append_child(ExprParser(parser, false).parse().node);
        }

        return variant.build(AST_ENUM_VARIANT);
    });

    node.append_child(result.node);
    return node.build(AST_ENUM_DEF);
}

ParseResult DeclParser::parse_union() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'union'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    ParseResult result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(AST_UNION_DEF);
}

ParseResult DeclParser::parse_union_case() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'case'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(parser.consume_into_node(AST_IDENTIFIER));
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'('");
        return node.build_error();
    }

    ParseResult result = parser.parse_param_list();
    node.append_child(result.node);
    if (!result.is_valid) {
        return node.build(AST_UNION_CASE);
    }

    return parser.check_stmt_terminator(node, AST_UNION_CASE);
}

ParseResult DeclParser::parse_proto() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'proto'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    ParseResult result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(AST_PROTO_DEF);
}

ParseResult DeclParser::parse_type_alias() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'type'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    if (!stream.get()->is(TKN_EQ)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'='");
        return node.build_error();
    }
    node.consume(); // Consume '='

    ParseResult result = parser.parse_type();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    return parser.check_stmt_terminator(node, AST_TYPE_ALIAS_DEF);
}

ParseResult DeclParser::parse_use() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'use'

    ParseResult result = parse_use_tree();
    node.append_child(result.node);

    if (!result.is_valid) {
        return {node.build(AST_USE_DECL), false};
    }

    return parser.check_stmt_terminator(node, AST_USE_DECL);
}

ParseResult DeclParser::parse_use_tree() {
    ParseResult result = parse_use_tree_element();
    if (!result.is_valid) {
        return result;
    }

    ASTNode *current_node = result.node;

    while (stream.get()->is(TKN_DOT)) {
        ASTNode *dot_operator = parser.create_node(AST_DOT_EXPR);
        dot_operator->append_child(current_node);
        stream.consume(); // Consume '.'
        dot_operator->tokens = parser.mod->create_token_index(stream.get_position() - 1);

        result = parse_use_tree_element();
        dot_operator->append_child(result.node);
        dot_operator->set_range_from_children();
        current_node = dot_operator;

        if (!result.is_valid) {
            return {current_node, false};
        }
    }

    return current_node;
}

ParseResult DeclParser::parse_use_tree_element() {
    if (parser.is_at_completion_point()) {
        return parser.parse_completion_point();
    }

    Token *token = stream.get();

    if (token->is(TKN_IDENTIFIER)) {
        ASTNode *identifier = parser.consume_into_node(AST_IDENTIFIER);

        if (!stream.get()->is(TKN_AS)) {
            return identifier;
        } else {
            unsigned as_token_index = stream.get_position();
            stream.consume(); // Consume 'as'

            ASTNode *rebinding_node = parser.create_node(AST_USE_REBIND);
            rebinding_node->append_child(identifier);
            rebinding_node->append_child(parser.consume_into_node(AST_IDENTIFIER));
            rebinding_node->set_range_from_children();
            rebinding_node->tokens = parser.mod->create_token_index(as_token_index);
            return rebinding_node;
        }
    } else if (token->is(TKN_LBRACE)) {
        return parser.parse_list(AST_USE_LIST, TKN_RBRACE, std::bind(&DeclParser::parse_use_tree, this));
    } else {
        parser.report_unexpected_token();
        return {parser.create_node(AST_ERROR), false};
    }
}

ParseResult DeclParser::parse_qualifiers() {
    ASTNode *qualifier_list = parser.create_node(AST_QUALIFIER_LIST);
    while (stream.get()->is(TKN_PUB)) {
        qualifier_list->append_child(parser.consume_into_node(AST_QUALIFIER));
    }

    return qualifier_list;
}

ParseResult DeclParser::parse_native() {
    Token *deciding_token = stream.peek(1);
    if (deciding_token->is(TKN_VAR)) {
        return parse_native_var();
    } else if (deciding_token->is(TKN_FUNC)) {
        return parse_native_func();
    } else {
        stream.consume(); // Consume 'native'
        parser.report_unexpected_token();
        stream.consume(); // Consume invalid token
        return {parser.create_node(AST_ERROR), false};
    }
}

ParseResult DeclParser::parse_native_var() {
    NodeBuilder node = parser.build_node();

    node.consume(); // Consume 'native'
    node.consume(); // Consume 'var'

    ASTNode *identifier = parser.consume_into_node(AST_IDENTIFIER);
    node.consume(); // Consume ':'
    ASTNode *data_type = parser.parse_type().node;

    node.append_child(identifier);
    node.append_child(data_type);

    return parser.check_stmt_terminator(node, AST_NATIVE_VAR_DECL);
}

ParseResult DeclParser::parse_native_func() {
    NodeBuilder node = parser.build_node();

    node.consume(); // Consume 'native'
    node.append_child(parser.create_node(AST_QUALIFIER_LIST));

    node.consume(); // Consume 'func'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    bool generic;
    bool head_valid = parse_func_head(node, generic);
    if (!head_valid) {
        return node.build_error();
    }

    return parser.check_stmt_terminator(node, AST_NATIVE_FUNC_DECL);
}

ParseResult DeclParser::parse_pub() {
    ASTNode *qualifier_list = nullptr;
    if (stream.get()->is(TKN_PUB)) {
        qualifier_list = parse_qualifiers().node;
    }

    switch (stream.get()->type) {
        case TKN_FUNC: return parse_func(qualifier_list);
        case TKN_CONST: return parse_const();
        case TKN_STRUCT: return parse_struct();
        case TKN_ENUM: return parse_enum();
        case TKN_UNION: return parse_union();
        case TKN_CASE: return parse_union_case();
        case TKN_PROTO: return parse_proto();
        case TKN_TYPE: return parse_type_alias();
        case TKN_NATIVE: return parse_native();
        case TKN_USE: return parse_use();
        default: parser.report_unexpected_token(); return {parser.create_node(AST_ERROR), false};
    }
}

bool DeclParser::parse_func_head(NodeBuilder &node, bool &generic) {
    generic = false;
    bool valid = true;

    if (stream.get()->is(TKN_LBRACKET)) {
        generic = true;

        ParseResult result = parse_generic_param_list();
        node.append_child(result.node);

        if (!result.is_valid) {
            node.append_child(parser.create_node(AST_PARAM_LIST));
            node.append_child(parser.create_node(AST_EMPTY));
            return false;
        }

        node.append_child(result.node);
    }

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'('");
        node.append_child(parser.create_node(AST_PARAM_LIST));
        node.append_child(parser.create_node(AST_EMPTY));
        return false;
    }

    ParseResult result = parser.parse_param_list();
    node.append_child(result.node);
    if (!result.is_valid) {
        valid = false;
    }

    if (stream.get()->is(TKN_ARROW)) {
        node.consume(); // Consume '->'

        ParseResult result = parser.parse_return_type();
        node.append_child(result.node);

        if (!result.is_valid) {
            return false;
        }
    } else if (stream.get()->is(TKN_LBRACE) || stream.get()->is(TKN_SEMI) || stream.previous()->end_of_line) {
        node.append_child(parser.create_node(AST_EMPTY));
    } else {
        parser.report_unexpected_token();
        node.append_child(parser.create_node(AST_EMPTY));
        return false;
    }

    return valid;
}

ParseResult DeclParser::parse_generic_param_list() {
    return parser.parse_list(AST_GENERIC_PARAM_LIST, TKN_RBRACKET, [this]() {
        NodeBuilder node = parser.build_node();
        node.append_child(parser.consume_into_node(AST_IDENTIFIER));

        if (stream.get()->is(TKN_COLON)) {
            node.consume(); // Consume ':'

            ParseResult result = parser.parse_type();
            if (result.is_valid) {
                node.append_child(result.node);
            } else {
                node.append_child(parser.create_node(AST_ERROR));
            }
        }

        return node.build(AST_GENERIC_PARAMETER);
    });
}

} // namespace lang

} // namespace banjo
