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
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume operator
    node.append_child(lhs_node);

    ParseResult result = ExprParser(parser, true).parse();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error(type);
    }

    return parser.check_stmt_terminator(node, type);
}

ParseResult StmtParser::parse_var() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'var'
    return parse_var_or_ref(node, AST_VAR_DEF, AST_TYPELESS_VAR_DEF);
}

ParseResult StmtParser::parse_ref() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'ref'

    if (stream.get()->is(TKN_MUT)) {
        node.consume(); // Consume 'mut'
        return parse_var_or_ref(node, AST_REF_MUT_VAR_DEF, AST_TYPELESS_REF_MUT_VAR_DEF);
    } else {
        return parse_var_or_ref(node, AST_REF_VAR_DEF, AST_TYPELESS_REF_VAR_DEF);
    }
}

ParseResult StmtParser::parse_var_or_ref(NodeBuilder &node, ASTNodeType type, ASTNodeType type_typeless) {
    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

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
    node.consume(); // Consume ':'

    ParseResult result = parser.parse_type();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    if (stream.get()->is(TKN_EQ)) {
        node.consume(); // Consume '='

        result = ExprParser(parser, true).parse();
        node.append_child(result.node);

        if (!result.is_valid) {
            return node.build_error(type);
        }
    }

    return parser.check_stmt_terminator(node, type);
}

ParseResult StmtParser::parse_var_without_type(NodeBuilder &node, ASTNodeType type) {
    node.consume(); // Consume '='

    ParseResult result = ExprParser(parser, true).parse();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error(type);
    }

    return parser.check_stmt_terminator(node, type);
}

ParseResult StmtParser::parse_if_chain() {
    NodeBuilder node = parser.build_node();

    NodeBuilder first_if = parser.build_node();
    first_if.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser).parse();
    first_if.append_child(result.node);
    if (!result.is_valid) {
        first_if.append_child(parser.create_node(AST_ERROR));
        node.append_child(first_if.build_with_inferred_range(AST_IF_BRANCH));
        return {node.build_with_inferred_range(AST_IF_STMT), false};
    }

    result = parser.parse_block();
    first_if.append_child(result.node);
    if (!result.is_valid) {
        node.append_child(first_if.build_with_inferred_range(AST_IF_BRANCH));
        return {node.build_with_inferred_range(AST_IF_STMT), false};
    }

    node.append_child(first_if.build(AST_IF_BRANCH));

    while (stream.get()->is(TKN_ELSE)) {
        if (stream.peek(1)->is(TKN_IF)) {
            NodeBuilder else_if_node = parser.build_node();
            else_if_node.consume(); // Consume 'else'
            else_if_node.consume(); // Consume 'if'

            result = ExprParser(parser).parse();
            else_if_node.append_child(result.node);
            if (!result.is_valid) {
                else_if_node.append_child(parser.create_node(AST_ERROR));
                node.append_child(else_if_node.build_with_inferred_range(AST_ELSE_IF_BRANCH));
                return {node.build_with_inferred_range(AST_IF_BRANCH), false};
            }

            else_if_node.append_child(parser.parse_block().node);
            node.append_child(else_if_node.build(AST_ELSE_IF_BRANCH));
        } else if (stream.peek(1)->is(TKN_LBRACE)) {
            NodeBuilder else_node = parser.build_node();
            else_node.consume(); // Consume 'else'
            else_node.append_child(parser.parse_block().node);
            node.append_child(else_node.build(AST_ELSE_BRANCH));
        } else {
            stream.consume(); // Consume 'else'
            parser.report_unexpected_token();
            return node.build_error();
        }
    }

    return node.build(AST_IF_STMT);
}

ParseResult StmtParser::parse_switch() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'switch'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    NodeBuilder cases_node = parser.build_node();
    cases_node.consume(); // Consume '{'

    while (stream.get()->is(TKN_CASE)) {
        NodeBuilder case_node = parser.build_node();
        case_node.consume(); // Consume 'case'

        if (stream.get()->value == "_") {
            stream.consume();
            case_node.append_child(parser.parse_block().node);
            cases_node.append_child(case_node.build(AST_SWITCH_DEFAULT_BRANCH));
            continue;
        }

        case_node.append_child(parser.consume_into_node(AST_IDENTIFIER));
        case_node.consume(); // Consume ':'

        result = parser.parse_type();
        if (result.is_valid) {
            case_node.append_child(result.node);
        } else {
            return node.build_error();
        }

        case_node.append_child(parser.parse_block().node);
        cases_node.append_child(case_node.build(AST_SWITCH_CASE_BRANCH));
    }

    cases_node.consume(); // Consume '}'
    node.append_child(cases_node.build(AST_SWITCH_CASE_LIST));

    return node.build(AST_SWITCH_STMT);
}

ParseResult StmtParser::parse_try() {
    NodeBuilder node = parser.build_node();
    NodeBuilder success_case_node = parser.build_node();
    success_case_node.consume(); // Consume 'try'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        success_case_node.append_child(parser.consume_into_node(AST_IDENTIFIER));
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (stream.get()->is(TKN_IN)) {
        success_case_node.consume(); // Consume 'in'
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
    node.append_child(success_case_node.build(AST_TRY_SUCCESS_BRANCH));

    if (stream.get()->is(TKN_EXCEPT)) {
        NodeBuilder error_case_node = parser.build_node();
        error_case_node.consume(); // Consume 'except'

        if (stream.get()->is(TKN_IDENTIFIER)) {
            error_case_node.append_child(parser.consume_into_node(AST_IDENTIFIER));
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
            return node.build_error();
        }

        error_case_node.consume(); // Consume ':'

        result = parser.parse_type();
        if (result.is_valid) {
            error_case_node.append_child(result.node);
        } else {
            return node.build_error();
        }

        error_case_node.append_child(parser.parse_block().node);
        node.append_child(error_case_node.build(AST_TRY_EXCEPT_BRANCH));
    } else if (stream.get()->is(TKN_ELSE)) {
        NodeBuilder else_case_node = parser.build_node();
        else_case_node.consume(); // Consume 'else'
        else_case_node.append_child(parser.parse_block().node);
        node.append_child(else_case_node.build(AST_TRY_ELSE_BRANCH));
    }

    return node.build(AST_TRY_STMT);
}

ParseResult StmtParser::parse_while() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'while'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    node.append_child(parser.parse_block().node);
    return node.build(AST_WHILE_STMT);
}

ParseResult StmtParser::parse_for() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'for'

    ASTNodeType type = AST_FOR_STMT;

    if (stream.get()->is(TKN_REF)) {
        node.consume(); // Consume 'ref'
        type = AST_FOR_REF_STMT;

        if (stream.get()->is(TKN_MUT)) {
            node.consume(); // Consume 'mut'
            type = AST_FOR_REF_MUT_STMT;
        }
    }

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(parser.consume_into_node(AST_IDENTIFIER));
    } else {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    if (stream.get()->is(TKN_IN)) {
        node.consume(); // Consume 'in'
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
    NodeBuilder builder = parser.build_node();
    builder.consume(); // Consume 'break'
    return parser.check_stmt_terminator(builder, AST_BREAK_STMT);
}

ParseResult StmtParser::parse_continue() {
    NodeBuilder builder = parser.build_node();
    builder.consume(); // Consume 'continue'
    return parser.check_stmt_terminator(builder, AST_CONTINUE_STMT);
}

ParseResult StmtParser::parse_return() {
    NodeBuilder builder = parser.build_node();
    builder.consume(); // Consume 'return'

    if (!stream.get()->is(TKN_SEMI) && !stream.previous()->end_of_line) {
        ParseResult result = ExprParser(parser, true).parse();
        builder.append_child(result.node);

        if (!result.is_valid) {
            return {builder.build(AST_RETURN_STMT), false};
        }
    }

    return parser.check_stmt_terminator(builder, AST_RETURN_STMT);
}

ParseResult StmtParser::parse_meta_stmt() {
    NodeBuilder node = parser.build_node();
    node.consume(); // Consume 'meta'

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
    first_branch.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser, false).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    first_branch.append_child(result.node);
    first_branch.append_child(parser.parse_block().node);
    node.append_child(first_branch.build(AST_META_IF_BRANCH));

    while (stream.get()->is(TKN_ELSE)) {
        NodeBuilder branch = parser.build_node();
        branch.consume(); // Consume 'else'

        if (stream.get()->is(TKN_IF)) {
            branch.consume(); // Consume 'if'

            ParseResult result = ExprParser(parser, false).parse();
            if (!result.is_valid) {
                return node.build_error();
            }
            branch.append_child(result.node);
            branch.append_child(parser.parse_block().node);
            node.append_child(branch.build(AST_META_ELSE_IF_BRANCH));
        } else if (stream.get()->is(TKN_LBRACE)) {
            branch.append_child(parser.parse_block().node);
            node.append_child(branch.build(AST_META_ELSE_BRANCH));
        } else {
            parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_UNEXPECTED);
            return node.build_error();
        }
    }

    return node.build(AST_META_IF_STMT);
}

ParseResult StmtParser::parse_meta_for(NodeBuilder &node) {
    node.consume(); // Consume 'for'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(parser.consume_into_node(AST_IDENTIFIER));

    if (!stream.get()->is(TKN_IN)) {
        parser.report_unexpected_token();
        return node.build_error();
    }
    node.consume(); // Consume 'in'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    node.append_child(parser.parse_block().node);

    return node.build(AST_META_FOR_STMT);
}

} // namespace lang

} // namespace banjo
