#include "decl_parser.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "ast/expr.hpp"
#include "parser/expr_parser.hpp"
#include "reports/report_texts.hpp"

namespace lang {

const std::unordered_set<TokenType> DeclParser::RECOVERY_TOKENS{
    TKN_RBRACE,
    TKN_EOF,
    TKN_FUNC,
    TKN_CONST,
    TKN_STRUCT,
    TKN_ENUM,
    TKN_UNION,
    TKN_PUB,
    TKN_NATIVE,
};

DeclParser::DeclParser(Parser &parser) : parser(parser), stream(parser.stream) {}

ASTNode *DeclParser::parse() {
    Token *token = stream.get();

    ASTNode *qualifier_list = nullptr;
    if (token->is(TKN_PUB)) {
        qualifier_list = parse_qualifiers().node;
        token = stream.get();
    }

    ParseResult result;
    switch (token->get_type()) {
        case TKN_FUNC: result = parse_func(qualifier_list); break;
        case TKN_CONST: result = parse_const(); break;
        case TKN_STRUCT: result = parse_struct(); break;
        case TKN_ENUM: result = parse_enum(); break;
        case TKN_UNION: result = parse_union(); break;
        case TKN_CASE: result = parse_union_case(); break;
        case TKN_TYPE: result = parse_type_alias(); break;
        case TKN_NATIVE: result = parse_native(); break;
        default: parser.report_unexpected_token(); result = {new ASTNode(AST_ERROR), false};
    }

    if (!result.is_valid) {
        recover();
    }

    return result.node;
}

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
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new Identifier(stream.consume()));

    bool generic;
    bool head_valid = parse_func_head(node, TKN_LBRACE, generic);

    if (!stream.get()->is(TKN_LBRACE)) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED, "'{'");

        node.append_child(parser.create_dummy_block());
        if (generic) {
            return {node.build<ASTGenericFunc>(), false};
        } else {
            return {node.build(new ASTFunc(AST_FUNCTION_DEFINITION)), false};
        }
    }

    ParseResult result = parser.parse_block(true);
    node.append_child(result.node);

    bool is_valid = head_valid && result.is_valid;
    if (generic) {
        return {node.build<ASTGenericFunc>(), is_valid};
    } else {
        return {node.build(new ASTFunc(AST_FUNCTION_DEFINITION)), is_valid};
    }
}

ParseResult DeclParser::parse_const() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'const'
    node.append_child(new Identifier(stream.consume()));
    stream.consume(); // Consume ':'
    node.append_child(parser.parse_type().node);
    stream.consume(); // Consume '='
    node.append_child(parser.parse_expression());

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_SEMI);
        return node.build_error();
    }

    return node.build<ASTConst>();
}

ParseResult DeclParser::parse_struct() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'struct'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new Identifier(stream.consume()));

    if (stream.get()->is(TKN_LBRACKET)) {
        node.append_child(parser.parse_generic_param_list());

        ParseResult result = parser.parse_block(true);
        node.append_child(result.node);

        if (!result.is_valid) {
            return {node.build<ASTGenericStruct>(), false};
        }
        return node.build<ASTGenericStruct>();
    } else {
        ParseResult result = parser.parse_block(true);
        node.append_child(result.node);

        if (!result.is_valid) {
            return {node.build<ASTStruct>(), false};
        }
        return node.build<ASTStruct>();
    }
}

ParseResult DeclParser::parse_enum() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'enum'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new Identifier(stream.consume()));

    ParseResult result = parser.parse_list(AST_ENUM_VARIANT_LIST, TKN_RBRACE, [this](NodeBuilder &) {
        NodeBuilder variant = parser.new_node();
        variant.append_child(new Identifier(stream.consume()));

        if (stream.get()->is(TKN_EQ)) {
            stream.consume(); // Consume '='
            variant.append_child(ExprParser(parser, false).parse().node);
        }

        return variant.build<ASTEnumVariant>();
    });

    node.append_child(result.node);
    return node.build<ASTEnum>();
}

ParseResult DeclParser::parse_union() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'union'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new Identifier(stream.consume()));

    ParseResult result = parser.parse_block(true);
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    return node.build<ASTUnion>();
}

ParseResult DeclParser::parse_union_case() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'case'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(new Identifier(stream.consume()));
    } else {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED, "'('");
        return node.build_error();
    }

    ParseResult result = parser.parse_param_list();
    node.append_child(result.node);
    if (!result.is_valid) {
        return node.build<ASTUnionCase>();
    }

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED, "';'");
    }

    return node.build<ASTUnionCase>();
}

ParseResult DeclParser::parse_type_alias() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'type'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new Identifier(stream.consume()));

    if (!stream.get()->is(TKN_EQ)) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED, "'='");
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
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_SEMI);
    }

    return (ASTNode *)node.build<ASTTypeAlias>();
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

    ASTNode *identifier = new Identifier(stream.consume());
    stream.consume(); // Consume ':'
    ASTNode *data_type = parser.parse_type().node;

    node.append_child(identifier);
    node.append_child(data_type);

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
    } else {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_SEMI);
        return node.build_error();
    }

    return node.build(new ASTVar(AST_NATIVE_VAR));
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
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new Identifier(stream.consume()));

    bool generic;
    bool head_valid = parse_func_head(node, TKN_SEMI, generic);
    if (!head_valid) {
        return node.build_error();
    }

    stream.consume(); // Consume ';'
    return (ASTNode *)node.build(new ASTFunc(AST_NATIVE_FUNCTION_DECLARATION));
}

bool DeclParser::parse_func_head(NodeBuilder &node, TokenType terminator, bool &generic) {
    generic = false;
    bool valid = true;

    if (stream.get()->is(TKN_LBRACKET)) {
        generic = true;
        node.append_child(parser.parse_generic_param_list());
    }

    if (!stream.get()->is(TKN_LPAREN)) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED, "'('");
        node.append_child(new ASTNode(AST_PARAM_LIST));
        node.append_child(new Expr(AST_VOID));
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
    } else if (stream.get()->is(terminator)) {
        node.append_child(new Expr(AST_VOID));
    } else {
        parser.report_unexpected_token();
        node.append_child(new Expr(AST_VOID));
        return false;
    }

    return valid;
}

void DeclParser::recover() {
    while (true) {
        Token *token = stream.get();

        if (RECOVERY_TOKENS.count(token->get_type()) || token->get_type() == TKN_EOF) {
            break;
        }

        stream.consume();
    }
}

} // namespace lang
