#include "stmt_parser.hpp"

#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "lexer/token.hpp"
#include "parser/expr_parser.hpp"
#include "parser/node_builder.hpp"
#include "reports/report_texts.hpp"

namespace banjo {

namespace lang {

StmtParser::StmtParser(Parser &parser) : parser(parser), stream(parser.stream) {}

ParseResult StmtParser::parse_assign(ASTNode *lhs_node, ASTNodeType type) {
    stream.consume(); // Consume operator

    ASTNode *node = new ASTNode(type);
    node->append_child(lhs_node);

    ParseResult result = ExprParser(parser, true).parse();
    node->append_child(result.node);
    node->set_range_from_children();

    if (!result.is_valid) {
        return {node, false};
    }

    return parser.check_semi(node);
}

ParseResult StmtParser::parse_var() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'var'

    if (stream.get()->get_type() != TKN_IDENTIFIER) {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }

    node.append_child(new Identifier(stream.consume()));

    if (stream.get()->is(TKN_COLON)) {
        return parse_var_with_type(node);
    } else if (stream.get()->is(TKN_EQ)) {
        return parse_var_without_type(node);
    } else {
        parser.report_unexpected_token();
        return node.build_error();
    }
}

ParseResult StmtParser::parse_var_with_type(NodeBuilder &node) {
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

    if (parser.current_attr_list) {
        node.set_attribute_list(parser.current_attr_list);
        parser.current_attr_list = nullptr;
    }

    return parser.check_semi(node.build(new ASTVar(AST_VAR)));
}

ParseResult StmtParser::parse_var_without_type(NodeBuilder &node) {
    stream.consume(); // Consume '='

    ParseResult result = ExprParser(parser, true).parse();
    node.append_child(result.node);

    if (!result.is_valid) {
        return node.build_error();
    }

    if (parser.current_attr_list) {
        node.set_attribute_list(parser.current_attr_list);
        parser.current_attr_list = nullptr;
    }

    return parser.check_semi(node.build(new ASTVar(AST_IMPLICIT_TYPE_VAR)));
}

ParseResult StmtParser::parse_if_chain() {
    NodeBuilder node = parser.new_node();

    NodeBuilder first_if = parser.new_node();
    stream.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser).parse();
    first_if.append_child(result.node);
    if (!result.is_valid) {
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
            NodeBuilder else_if_node = parser.new_node();
            stream.consume(); // Consume 'else'
            stream.consume(); // Consume 'if'

            result = ExprParser(parser).parse();
            else_if_node.append_child(result.node);
            if (!result.is_valid) {
                node.append_child(else_if_node.build_with_inferred_range(AST_ELSE_IF));
                return {node.build_with_inferred_range(AST_IF), false};
            }

            else_if_node.append_child(parser.parse_block().node);
            node.append_child(else_if_node.build(AST_ELSE_IF));
        } else if (stream.peek(1)->is(TKN_LBRACE)) {
            NodeBuilder else_node = parser.new_node();
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
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'switch'

    ParseResult result = ExprParser(parser).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    node.append_child(result.node);

    NodeBuilder cases_node = parser.new_node();
    stream.consume(); // Consume '{'

    while (stream.get()->is(TKN_CASE)) {
        NodeBuilder case_node = parser.new_node();
        stream.consume(); // Consume 'case'

        Token *ident_token = stream.consume();
        if (ident_token->get_value() == "_") {
            case_node.append_child(parser.parse_block().node);
            cases_node.append_child(case_node.build(AST_SWITCH_DEFAULT_CASE));
            continue;
        }

        case_node.append_child(new Identifier(ident_token));
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
    NodeBuilder node = parser.new_node();
    NodeBuilder success_case_node = parser.new_node();
    stream.consume(); // Consume 'try'

    if (stream.get()->is(TKN_IDENTIFIER)) {
        success_case_node.append_child(new Identifier(stream.consume()));
    } else {
        parser.report_unexpected_token(ReportText::ERR_PARSE_EXPECTED_IDENTIFIER);
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
        NodeBuilder error_case_node = parser.new_node();
        stream.consume(); // Consume 'except'

        if (stream.get()->is(TKN_IDENTIFIER)) {
            error_case_node.append_child(new Identifier(stream.consume()));
        } else {
            parser.report_unexpected_token(ReportText::ERR_PARSE_EXPECTED_IDENTIFIER);
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
        NodeBuilder else_case_node = parser.new_node();
        stream.consume(); // Consume 'else'
        else_case_node.append_child(parser.parse_block().node);
        node.append_child(else_case_node.build(AST_TRY_ELSE_CASE));
    }

    return node.build(AST_TRY);
}

ParseResult StmtParser::parse_while() {
    NodeBuilder node = parser.new_node();
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
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'for'

    if (stream.get()->is(TKN_STAR)) {
        node.append_child(new ASTNode(AST_FOR_ITER_TYPE, stream.consume()));
    } else {
        node.append_child(new ASTNode(AST_FOR_ITER_TYPE, ""));
    }

    if (stream.get()->is(TKN_IDENTIFIER)) {
        node.append_child(new Identifier(stream.consume()));
    } else {
        parser.report_unexpected_token(ReportText::ERR_PARSE_EXPECTED_IDENTIFIER);
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

    return node.build(AST_FOR);
}

ParseResult StmtParser::parse_break() {
    TextRange range = stream.consume()->get_range();
    return parser.check_semi(new ASTNode(AST_BREAK, range));
}

ParseResult StmtParser::parse_continue() {
    TextRange range = stream.consume()->get_range();
    return parser.check_semi(new ASTNode(AST_CONTINUE, range));
}

ParseResult StmtParser::parse_return() {
    NodeBuilder builder = parser.new_node();
    stream.consume(); // Consume 'return'

    if (!stream.get()->is(TKN_SEMI)) {
        ParseResult result = ExprParser(parser, true).parse();
        builder.append_child(result.node);

        if (!result.is_valid) {
            return {builder.build(AST_FUNCTION_RETURN), false};
        }
    }

    return parser.check_semi(builder.build(AST_FUNCTION_RETURN));
}

ParseResult StmtParser::parse_meta_stmt() {
    NodeBuilder node = parser.new_node();
    stream.consume(); // Consume 'meta'

    if (stream.get()->is(TKN_IF)) {
        return parse_meta_if(node);
    } else if (stream.get()->is(TKN_FOR)) {
        return parse_meta_for(node);
    } else {
        parser.report_unexpected_token(ReportText::ID::ERR_PARSE_UNEXPECTED);
        return node.build_error();
    }
}

ParseResult StmtParser::parse_meta_if(NodeBuilder &node) {
    NodeBuilder first_branch = parser.new_node();
    stream.consume(); // Consume 'if'

    ParseResult result = ExprParser(parser, false).parse();
    if (!result.is_valid) {
        return node.build_error();
    }
    first_branch.append_child(result.node);
    first_branch.append_child(parser.parse_block().node);
    node.append_child(first_branch.build(AST_META_IF_CONDITION));

    while (stream.get()->is(TKN_ELSE)) {
        NodeBuilder branch = parser.new_node();
        stream.consume(); // Consume 'else'

        if (stream.get()->is(TKN_IF)) {
            stream.consume(); // Consume 'if'

            ParseResult result = ExprParser(parser, false).parse();
            if (!result.is_valid) {
                return node.build_error();
            }
            branch.append_child(result.node);
            branch.append_child(parser.parse_block(false).node);
            node.append_child(branch.build(AST_META_IF_CONDITION));
        } else if (stream.get()->is(TKN_LBRACE)) {
            branch.append_child(parser.parse_block(false).node);
            node.append_child(branch.build(AST_META_ELSE));
        } else {
            parser.report_unexpected_token(ReportText::ERR_PARSE_UNEXPECTED);
            return node.build_error();
        }
    }

    return node.build(AST_META_IF);
}

ParseResult StmtParser::parse_meta_for(NodeBuilder &node) {
    stream.consume(); // Consume 'for'

    if (!stream.get()->is(TKN_IDENTIFIER)) {
        parser.report_unexpected_token(ReportText::ERR_PARSE_EXPECTED_IDENTIFIER);
        return node.build_error();
    }
    node.append_child(new Identifier(stream.consume()));

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

    node.append_child(parser.parse_block(false).node);

    return node.build(AST_META_FOR);
}

} // namespace lang

} // namespace banjo
