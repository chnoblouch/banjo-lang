#include "stmt_parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/expr_parser.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/source/text_range.hpp"

namespace banjo {

namespace lang {

StmtParser::StmtParser(Parser &parser) : parser(parser), stream(parser.stream) {}

ParseResult StmtParser::parse_assign(ASTNode *lhs_node, ASTNodeType type) {
    stream.consume(); // Consume operator

    ASTNode *node = parser.create_node(type);
    node->append_child(lhs_node);

    ParseResult result = ExprParser(parser, true).parse();
    node->append_child(result.node);
    node->set_range_from_children();

    if (!result.is_valid) {
        return {node, false};
    }

    return parser.check_stmt_terminator(node);
}

ParseResult StmtParser::parse_var() {
    TextPosition start = stream.consume()->position; // Consume 'var'
    return parse_var_or_ref(start, AST_VAR, AST_IMPLICIT_TYPE_VAR);
}

ParseResult StmtParser::parse_ref() {
    TextPosition start = stream.consume()->position; // Consume 'ref'

    if (stream.get()->is(TKN_MUT)) {
        stream.consume(); // Consume 'mut'
        return parse_var_or_ref(start, AST_REF_MUT_VAR, AST_IMPLICIT_TYPE_REF_MUT_VAR);
    } else {
        return parse_var_or_ref(start, AST_REF_VAR, AST_IMPLICIT_TYPE_REF_VAR);
    }
}

ParseResult StmtParser::parse_var_or_ref(TextPosition start, ASTNodeType type, ASTNodeType type_typeless) {
    NodeBuilder node = parser.build_node();
    node.set_start_position(start);

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(parser.create_node(AST_IDENTIFIER, stream.consume()));

    if (stream.get()->is(TKN_COLON)) {
        return parse_var_with_type(node, type);
    } else if (stream.get()->is(TKN_EQ)) {
        return parse_var_without_type(node, type_typeless);
    } else {
        parser.report_unexpected_token();
        return node.build_error();
    }
}

ParseResult StmtParser::parse_var_with_type(NodeBuilder &node, ASTNodeType type) {
    stream.consume(); // Consume ':'

    ParseResult result = parser.parse_type();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    if (stream.get()->is(TKN_EQ)) {
        stream.consume(); // Consume '='

        result = ExprParser(parser, true).parse();
        node.append_child(result.node);

        if (!result.is_valid) {
            return node.build_error();
        }
    }

    return parser.check_stmt_terminator(node.build(type));
}

ParseResult StmtParser::parse_var_without_type(NodeBuilder &node, ASTNodeType type) {
    stream.consume(); // Consume '='

    ParseResult result = ExprParser(parser, true).parse();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    return parser.check_stmt_terminator(node.build(type));
}

ParseResult StmtParser::parse_if_chain() {
    NodeBuilder node = parser.build_node();

    NodeBuilder first_if = parser.build_node();
    stream.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser).parse();
    first_if.append_child(result.node);
    if (!result.is_valid) {
        first_if.append_child(parser.create_node(AST_ERROR));
        node.append_child(first_if.build_with_inferred_range(AST_IF));
        return {node.build_with_inferred_range(AST_IF_CHAIN), false};
    }

    result = parser.parse_block();
    first_if.append_child(result.node);
    if (!result.is_valid) {
        node.append_child(first_if.build_with_inferred_range(AST_IF));
        return {node.build_with_inferred_range(AST_IF_CHAIN), false};
    }

    node.append_child(first_if.build(AST_IF));

    while (stream.get()->is(TKN_ELSE)) {
        if (stream.peek(1)->is(TKN_IF)) {
            NodeBuilder else_if_node = parser.build_node();
            stream.consume(); // Consume 'else'
            stream.consume(); // Consume 'if'

            result = ExprParser(parser).parse();
            else_if_node.append_child(result.node);
            if (!result.is_valid) {
                else_if_node.append_child(parser.create_node(AST_ERROR));
                node.append_child(else_if_node.build_with_inferred_range(AST_ELSE_IF));
                return {node.build_with_inferred_range(AST_IF), false};
            }

            else_if_node.append_child(parser.parse_block().node);
            node.append_child(else_if_node.build(AST_ELSE_IF));
        } else if (stream.peek(1)->is(TKN_LBRACE)) {
            NodeBuilder else_node = parser.build_node();
            stream.consume(); // Consume 'else'
            else_node.append_child(parser.parse_block().node);
            node.append_child(else_node.build(AST_ELSE));
        } else {
            stream.consume(); // Consume 'else'
            parser.report_unexpected_token();
            return node.build_error();
        }
    }

    return node.build(AST_IF_CHAIN);
}

ParseResult StmtParser::parse_switch() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'switch'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    NodeBuilder cases_node = parser.build_node();
    stream.consume(); // Consume '{'

    while (stream.get()->is(TKN_CASE)) {
        NodeBuilder case_node = parser.build_node();
        stream.consume(); // Consume 'case'

        Token *ident_token = stream.consume();
        if (ident_token->value == "_") {
            case_node.append_child(parser.parse_block().node);
            cases_node.append_child(case_node.build(AST_SWITCH_DEFAULT_CASE));
            continue;
        }

        case_node.append_child(parser.create_node(AST_IDENTIFIER, ident_token));
        stream.consume(); // Consume ':'

        result = parser.parse_type();
        if (result.is_valid) {
            case_node.append_child(result.node);
        } else {
            return node.build_error();
        }

        case_node.append_child(parser.parse_block().node);
        cases_node.append_child(case_node.build(AST_SWITCH_CASE));
    }

    stream.consume(); // Consume '}'
    node.append_child(cases_node.build(AST_SWITCH_CASE_LIST));

    return node.build(AST_SWITCH);
}

ParseResult StmtParser::parse_try() {
    NodeBuilder node = parser.build_node();
    NodeBuilder success_case_node = parser.build_node();
    stream.consume(); // Consume 'try'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        success_case_node.append_child(parser.create_node(AST_IDENTIFIER, stream.consume()));
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (stream.get()->is(TKN_IN)) {
        stream.consume(); // Consume 'in'
    } else {
        parser.report_unexpected_token();
        return node.build_error();
    }

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    success_case_node.append_child(result.node);

    success_case_node.append_child(parser.parse_block().node);
    node.append_child(success_case_node.build(AST_TRY_SUCCESS_CASE));

    if (stream.get()->is(TKN_EXCEPT)) {
        NodeBuilder error_case_node = parser.build_node();
        stream.consume(); // Consume 'except'

        if (stream.get()->is(TKN_IDENTIFIER)) {
            error_case_node.append_child(parser.create_node(AST_IDENTIFIER, stream.consume()));
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
            return node.build_error();
        }

        stream.consume(); // Consume ':'

        result = parser.parse_type();
        if (result.is_valid) {
            error_case_node.append_child(result.node);
        } else {
            return node.build_error();
        }

        error_case_node.append_child(parser.parse_block().node);
        node.append_child(error_case_node.build(AST_TRY_ERROR_CASE));
    } else if (stream.get()->is(TKN_ELSE)) {
        NodeBuilder else_case_node = parser.build_node();
        stream.consume(); // Consume 'else'
        else_case_node.append_child(parser.parse_block().node);
        node.append_child(else_case_node.build(AST_TRY_ELSE_CASE));
    }

    return node.build(AST_TRY);
}

ParseResult StmtParser::parse_while() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'while'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    node.append_child(parser.parse_block().node);
    return node.build(AST_WHILE);
}

ParseResult StmtParser::parse_for() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'for'

    ASTNodeType type = AST_FOR;

    if (stream.get()->is(TKN_REF)) {
        stream.consume(); // Consume 'ref'
        type = AST_FOR_REF;

        if (stream.get()->is(TKN_MUT)) {
            stream.consume(); // Consume 'mut'
            type = AST_FOR_REF_MUT;
        }
    }

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(parser.create_node(AST_IDENTIFIER, stream.consume()));
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (stream.get()->is(TKN_IN)) {
        stream.consume(); // Consume 'in'
    } else {
        parser.report_unexpected_token();
        return node.build_error();
    }

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    node.append_child(parser.parse_block().node);

    return node.build(type);
}

ParseResult StmtParser::parse_break() {
    TextRange range = stream.consume()->range();
    return parser.check_stmt_terminator(parser.create_node(AST_BREAK, range));
}

ParseResult StmtParser::parse_continue() {
    TextRange range = stream.consume()->range();
    return parser.check_stmt_terminator(parser.create_node(AST_CONTINUE, range));
}

ParseResult StmtParser::parse_return() {
    NodeBuilder builder = parser.build_node();
    stream.consume(); // Consume 'return'

    if (!stream.get()->is(TKN_SEMI) && !stream.previous()->end_of_line) {
        ParseResult result = ExprParser(parser, true).parse();
        builder.append_child(result.node);

        if (!result.is_valid) {
            return {builder.build(AST_FUNCTION_RETURN), false};
        }
    }

    return parser.check_stmt_terminator(builder.build(AST_FUNCTION_RETURN));
}

ParseResult StmtParser::parse_meta_stmt() {
    NodeBuilder node = parser.build_node();
    stream.consume(); // Consume 'meta'

    if (stream.get()->is(TKN_IF)) {
        return parse_meta_if(node);
    } else if (stream.get()->is(TKN_FOR)) {
        return parse_meta_for(node);
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_UNEXPECTED);
        return node.build_error();
    }
}

ParseResult StmtParser::parse_meta_if(NodeBuilder &node) {
    NodeBuilder first_branch = parser.build_node();
    stream.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser, false).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    first_branch.append_child(result.node);
    first_branch.append_child(parser.parse_block().node);
    node.append_child(first_branch.build(AST_META_IF_CONDITION));

    while (stream.get()->is(TKN_ELSE)) {
        NodeBuilder branch = parser.build_node();
        stream.consume(); // Consume 'else'

        if (stream.get()->is(TKN_IF)) {
            stream.consume(); // Consume 'if'

            ParseResult result = ExprParser(parser, false).parse();
            if (!result.is_valid) {
                return node.build_error();
            }
            branch.append_child(result.node);
            branch.append_child(parser.parse_block().node);
            node.append_child(branch.build(AST_META_IF_CONDITION));
        } else if (stream.get()->is(TKN_LBRACE)) {
            branch.append_child(parser.parse_block().node);
            node.append_child(branch.build(AST_META_ELSE));
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_UNEXPECTED);
            return node.build_error();
        }
    }

    return node.build(AST_META_IF);
}

ParseResult StmtParser::parse_meta_for(NodeBuilder &node) {
    stream.consume(); // Consume 'for'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.create_node(AST_IDENTIFIER, stream.consume()));

    if (!stream.get()->is(TKN_IN)) {
        parser.report_unexpected_token();
        return node.build_error();
    }
    stream.consume(); // Consume 'in'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    node.append_child(parser.parse_block().node);

    return node.build(AST_META_FOR);
}

} // namespace lang

} // namespace banjo
