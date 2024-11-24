#include "decl_parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/expr_parser.hpp"
#include "banjo/reports/report_texts.hpp"

namespace banjo {

namespace lang {

DeclParser::DeclParser(Parser &parser) : parser(parser), stream(parser.stream) {}

ParseResult DeclParser::parse_func(ASTNode *qualifier_list) {
    NodeBuilder node = parser.new_node();

    if (qualifier_list) {
        node.append_child(qualifier_list);
    } else {
        node.append_child(new ASTNode(AST_QUALIFIER_LIST));
    }

    if (parser.current_attr_list) {
        node.set_attribute_list(parser.current_attr_list);
        parser.current_attr_list = nullptr;
    }

    stream.consume(); // Consume 'func'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    bool generic;
    bool head_valid = parse_func_head(node, generic);

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
        return {node.build(AST_FUNC_DECL), head_valid};
    } else if (!stream.get()->is(TKN_LBRACE)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'{'");

        node.append_child(parser.create_dummy_block());
        if (generic) {
            return {node.build(new ASTNode(AST_GENERIC_FUNCTION_DEFINITION)), false};
        } else {
            return {node.build(new ASTNode(AST_FUNCTION_DEFINITION)), false};
        }
    }

    ParseResult result = parser.parse_block();
    node.append_child(result.node);

    bool is_valid = head_valid && result.is_valid;
    if (generic) {
        return {node.build(new ASTNode(AST_GENERIC_FUNCTION_DEFINITION)), is_valid};
    } else {
        return {node.build(new ASTNode(AST_FUNCTION_DEFINITION)), is_valid};
    }
}

ParseResult DeclParser::parse_const() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'const'
    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));
    stream.consume(); // Consume ':'
    node.append_child(parser.parse_type().node);
    stream.consume(); // Consume '='
    node.append_child(parser.parse_expression());

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_SEMI);
        return node.build_error();
    }

    return node.build(new ASTNode(AST_CONSTANT));
}

ParseResult DeclParser::parse_struct() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'struct'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    if (stream.get()->is(TKN_LBRACKET)) {
        ParseResult result = parse_generic_param_list();
        if (!result.is_valid) {
            return node.build_error();
        }

        node.append_child(result.node);

        result = parser.parse_block();
        node.append_child(result.node);

        if (!result.is_valid) {
            return {node.build(new ASTNode(AST_GENERIC_STRUCT_DEFINITION)), false};
        }
        return node.build(new ASTNode(AST_GENERIC_STRUCT_DEFINITION));
    }

    if (stream.get()->is(TKN_COLON)) {
        ParseResult result = parser.parse_list(
            AST_IMPL_LIST,
            TKN_LBRACE,
            [this](NodeBuilder &) { return ExprParser(parser).parse_type(); },
            false
        );

        if (result.is_valid) {
            node.append_child(result.node);
        } else {
            return node.build_error();
        }
    } else {
        node.append_child(new ASTNode(AST_IMPL_LIST));
    }

    ParseResult result = parser.parse_block();
    node.append_child(result.node);

    if (!result.is_valid) {
        return {node.build(new ASTNode(AST_STRUCT_DEFINITION)), false};
    }
    return node.build(new ASTNode(AST_STRUCT_DEFINITION));
}

ParseResult DeclParser::parse_enum() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'enum'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    ParseResult result = parser.parse_list(AST_ENUM_VARIANT_LIST, TKN_RBRACE, [this](NodeBuilder &) {
        NodeBuilder variant = parser.new_node();
        variant.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

        if (stream.get()->is(TKN_EQ)) {
            stream.consume(); // Consume '='
            variant.append_child(ExprParser(parser, false).parse().node);
        }

        return variant.build(new ASTNode(AST_ENUM_VARIANT));
    });

    node.append_child(result.node);
    return node.build(new ASTNode(AST_ENUM_DEFINITION));
}

ParseResult DeclParser::parse_union() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'union'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    ParseResult result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(new ASTNode(AST_UNION));
}

ParseResult DeclParser::parse_union_case() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'case'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));
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
        return node.build(new ASTNode(AST_UNION_CASE));
    }

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "';'");
    }

    return node.build(new ASTNode(AST_UNION_CASE));
}

ParseResult DeclParser::parse_proto() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'proto'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    ParseResult result = parser.parse_block();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build(new ASTNode(AST_PROTO));
}

ParseResult DeclParser::parse_type_alias() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'type'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    if (!stream.get()->is(TKN_EQ)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'='");
        return node.build_error();
    }
    stream.consume(); // Consume '='

    ParseResult result = parser.parse_type();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_SEMI);
    }

    return node.build(new ASTNode(AST_TYPE_ALIAS));
}

ParseResult DeclParser::parse_use() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'use'

    ParseResult result = parse_use_tree();
    node.append_child(result.node);

    if (!result.is_valid) {
        return {node.build(AST_USE), false};
    }

    return parser.check_semi(node.build(AST_USE));
}

ParseResult DeclParser::parse_use_tree() {
    ParseResult result = parse_use_tree_element();
    if (!result.is_valid) {
        return result;
    }

    ASTNode *current_node = result.node;

    while (stream.get()->is(TKN_DOT)) {
        ASTNode *dot_operator = new ASTNode(AST_DOT_OPERATOR);
        dot_operator->append_child(current_node);
        stream.consume(); // Consume '.'

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
    if (token->get_type() == TKN_IDENTIFIER) {
        ASTNode *identifier = new ASTNode(AST_IDENTIFIER, stream.consume());

        if (!stream.get()->is(TKN_AS)) {
            return identifier;
        } else {
            stream.consume(); // Consume 'as'

            ASTNode *rebinding_node = new ASTNode(AST_USE_REBINDING);
            rebinding_node->append_child(identifier);
            rebinding_node->append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));
            rebinding_node->set_range_from_children();
            return rebinding_node;
        }
    } else if (token->is(TKN_LBRACE)) {
        return parser.parse_list(AST_USE_TREE_LIST, TKN_RBRACE, [this](NodeBuilder &) { return parse_use_tree(); });
    } else {
        parser.report_unexpected_token();
        return {new ASTNode(AST_ERROR), false};
    }
}

ParseResult DeclParser::parse_qualifiers() {
    ASTNode *qualifier_list = new ASTNode(AST_QUALIFIER_LIST);
    while (stream.get()->is(TKN_PUB)) {
        qualifier_list->append_child(new ASTNode(AST_QUALIFIER, stream.consume()));
    }

    return qualifier_list;
}

ParseResult DeclParser::parse_native() {
    Token *deciding_token = stream.peek(1);
    if (deciding_token->is(TKN_VAR)) return parse_native_var();
    else if (deciding_token->is(TKN_FUNC)) return parse_native_func();
    else {
        stream.consume(); // Consume 'native'
        parser.report_unexpected_token();
        stream.consume(); // Consume invalid token
        return {new ASTNode(AST_ERROR), false};
    }
}

ParseResult DeclParser::parse_native_var() {
    NodeBuilder node = parser.new_node();

    if (parser.current_attr_list) {
        node.set_attribute_list(parser.current_attr_list);
        parser.current_attr_list = nullptr;
    }

    stream.consume(); // Consume native keyword
    stream.consume(); // Consume var keyword

    ASTNode *identifier = new ASTNode(AST_IDENTIFIER, stream.consume());
    stream.consume(); // Consume ':'
    ASTNode *data_type = parser.parse_type().node;

    node.append_child(identifier);
    node.append_child(data_type);

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_SEMI);
        return node.build_error();
    }

    return node.build(new ASTNode(AST_NATIVE_VAR));
}

ParseResult DeclParser::parse_native_func() {
    NodeBuilder node = parser.new_node();

    stream.consume(); // Consume 'native'
    node.append_child(new ASTNode(AST_QUALIFIER_LIST));

    if (parser.current_attr_list) {
        node.set_attribute_list(parser.current_attr_list);
        parser.current_attr_list = nullptr;
    }

    stream.consume(); // Consume 'func'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

    bool generic;
    bool head_valid = parse_func_head(node, generic);
    if (!head_valid) {
        return node.build_error();
    }

    stream.consume(); // Consume ';'
    return node.build(new ASTNode(AST_NATIVE_FUNCTION_DECLARATION));
}

ParseResult DeclParser::parse_pub() {
    ASTNode *qualifier_list = nullptr;
    if (stream.get()->is(TKN_PUB)) {
        qualifier_list = parse_qualifiers().node;
    }

    switch (stream.get()->get_type()) {
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
        default: parser.report_unexpected_token(); return {new ASTNode(AST_ERROR), false};
    }
}

bool DeclParser::parse_func_head(NodeBuilder &node, bool &generic) {
    generic = false;
    bool valid = true;

    if (stream.get()->is(TKN_LBRACKET)) {
        generic = true;

        ParseResult result = parse_generic_param_list();
        if (!result.is_valid) {
            node.append_child(new ASTNode(AST_PARAM_LIST));
            node.append_child(new ASTNode(AST_VOID));
            return false;
        }

        node.append_child(result.node);
    }

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'('");
        node.append_child(new ASTNode(AST_PARAM_LIST));
        node.append_child(new ASTNode(AST_VOID));
        return false;
    }

    ParseResult result = parser.parse_param_list();
    node.append_child(result.node);
    if (!result.is_valid) {
        valid = false;
    }

    if (stream.get()->is(TKN_ARROW)) {
        stream.consume(); // Consume '->'
        result = parser.parse_type();
        node.append_child(result.node);

        if (!result.is_valid) {
            return false;
        }
    } else if (stream.get()->is(TKN_LBRACE) || stream.get()->is(TKN_SEMI)) {
        node.append_child(new ASTNode(AST_VOID));
    } else {
        parser.report_unexpected_token();
        node.append_child(new ASTNode(AST_VOID));
        return false;
    }

    return valid;
}

ParseResult DeclParser::parse_generic_param_list() {
    return parser.parse_list(AST_GENERIC_PARAM_LIST, TKN_RBRACKET, [this](NodeBuilder &) {
        NodeBuilder node = parser.new_node();
        node.append_child(new ASTNode(AST_IDENTIFIER, stream.consume()));

        if (stream.get()->is(TKN_COLON)) {
            stream.consume(); // Consume ':'

            ParseResult result = parser.parse_type();
            if (result.is_valid) {
                node.append_child(result.node);
            } else {
                node.append_child(new ASTNode(AST_ERROR));
            }
        }

        return node.build(AST_GENERIC_PARAMETER);
    });
}

} // namespace lang

} // namespace banjo
