#include "parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/decl_parser.hpp"
#include "banjo/parser/expr_parser.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/parser/stmt_parser.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/utils/timing.hpp"

#include <unordered_set>
#include <utility>

namespace banjo {

namespace lang {

const std::unordered_set<TokenType> RECOVER_KEYWORDS{
    TKN_EOF,
    TKN_IF,
    TKN_WHILE,
    TKN_FOR,
    TKN_BREAK,
    TKN_CONTINUE,
    TKN_RETURN,
    TKN_VAR,
    TKN_CONST,
    TKN_FUNC,
    TKN_STRUCT,
    TKN_ENUM,
    TKN_UNION,
    TKN_PUB,
    TKN_NATIVE,
};

Parser::Parser(SourceFile &file, TokenList &input, Mode mode /*= Mode::COMPILATION*/)
  : file{file},
    stream{input},
    mode{mode} {}

void Parser::enable_completion() {
    running_completion = true;
}

std::unique_ptr<ASTModule> Parser::parse_module() {
    PROFILE_SCOPE("parser");

    stream.seek(0);

    mod = std::make_unique<ASTModule>(file);
    mod->append_child(parse_top_level_block());
    mod->set_range_from_children();
    return std::move(mod);
}

ASTNode *Parser::parse_top_level_block() {
    NodeBuilder node = build_node();

    while (!stream.get()->is(TKN_EOF)) {
        parse_and_append_block_child(node);
    }

    return node.build(AST_BLOCK);
}

ParseResult Parser::parse_block() {
    NodeBuilder node = build_node();

    if (stream.get()->is(TKN_LBRACE)) {
        node.consume(); // Consume '{'
    } else {
        report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'{'");
        return node.build_error();
    }

    while (true) {
        if (stream.get()->is(TKN_RBRACE)) {
            node.consume(); // Consume '}'
            break;
        } else if (stream.get()->is(TKN_EOF)) {
            register_error(stream.previous()->range()).set_message(ReportText("file ends with unclosed block").str());
            return node.build_error(AST_BLOCK);
        } else {
            parse_and_append_block_child(node);
        }
    }

    return node.build(AST_BLOCK);
}

void Parser::parse_and_append_block_child(NodeBuilder &node) {
    ParseResult result = parse_block_child();
    node.append_child(result.node);

    if (!result.is_valid) {
        recover();
    }
}

ParseResult Parser::parse_block_child() {
    switch (stream.get()->type) {
        case TKN_VAR: return StmtParser(*this).parse_var();
        case TKN_REF: return StmtParser(*this).parse_ref();
        case TKN_CONST: return DeclParser(*this).parse_const();
        case TKN_FUNC: return DeclParser(*this).parse_func(nullptr);
        case TKN_IF: return StmtParser(*this).parse_if_chain();
        case TKN_SWITCH: return StmtParser(*this).parse_switch();
        case TKN_TRY: return parse_try();
        case TKN_WHILE: return StmtParser(*this).parse_while();
        case TKN_FOR: return StmtParser(*this).parse_for();
        case TKN_BREAK: return StmtParser(*this).parse_break();
        case TKN_CONTINUE: return StmtParser(*this).parse_continue();
        case TKN_RETURN: return StmtParser(*this).parse_return();
        case TKN_PUB: return DeclParser(*this).parse_pub();
        case TKN_STRUCT: return DeclParser(*this).parse_struct();
        case TKN_ENUM: return DeclParser(*this).parse_enum();
        case TKN_UNION: return DeclParser(*this).parse_union();
        case TKN_PROTO: return DeclParser(*this).parse_proto();
        case TKN_TYPE: return DeclParser(*this).parse_type_alias();
        case TKN_CASE: return DeclParser(*this).parse_union_case();
        case TKN_SELF: return parse_expr_or_assign();
        case TKN_NATIVE: return DeclParser(*this).parse_native();
        case TKN_USE: return DeclParser(*this).parse_use();
        case TKN_META: return parse_meta();
        case TKN_LBRACE: return parse_block();
        case TKN_AT: return parse_attribute_wrapper(std::bind(&Parser::parse_block_child, this));
        default: return parse_expr_or_assign();
    }
}

ASTNode *Parser::parse_expression() {
    return ExprParser(*this).parse().node;
}

ParseResult Parser::parse_expr_or_assign() {
    NodeBuilder node = build_node();

    ParseResult result = ExprParser(*this, false).parse();
    if (!result.is_valid) {
        node.append_child(result.node);
        return node.build_error(AST_EXPR_STMT);
    }

    switch (stream.get()->type) {
        case TKN_EQ: return StmtParser(*this).parse_assign(result.node, AST_ASSIGNMENT);
        case TKN_PLUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_ADD_ASSIGN);
        case TKN_MINUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_SUB_ASSIGN);
        case TKN_STAR_EQ: return StmtParser(*this).parse_assign(result.node, AST_MUL_ASSIGN);
        case TKN_SLASH_EQ: return StmtParser(*this).parse_assign(result.node, AST_DIV_ASSIGN);
        case TKN_PERCENT_EQ: return StmtParser(*this).parse_assign(result.node, AST_MOD_ASSIGN);
        case TKN_AND_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_AND_ASSIGN);
        case TKN_OR_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_OR_ASSIGN);
        case TKN_CARET_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_XOR_ASSIGN);
        case TKN_SHL_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHL_ASSIGN);
        case TKN_SHR_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHR_ASSIGN);
        default: {
            node.append_child(result.node);
            return check_stmt_terminator(node, AST_EXPR_STMT);
        }
    }
}

ParseResult Parser::parse_try() {
    if (stream.peek(2)->is(TKN_IN)) {
        return StmtParser(*this).parse_try();
    } else {
        NodeBuilder node = build_node();
        ParseResult result = ExprParser(*this, false).parse();
        node.append_child(result.node);

        if (result.is_valid) {
            return check_stmt_terminator(node, AST_EXPR_STMT);
        } else {
            return node.build_error(AST_EXPR_STMT);
        }
    }
}

ParseResult Parser::parse_meta() {
    if (stream.peek(1)->is(TKN_IF) || stream.peek(1)->is(TKN_FOR)) {
        return StmtParser(*this).parse_meta_stmt();
    } else {
        return parse_expr_or_assign();
    }
}

ParseResult Parser::parse_type() {
    return ExprParser(*this).parse_type();
}

ParseResult Parser::parse_attribute_wrapper(const std::function<ParseResult()> &child_parser) {
    NodeBuilder node = build_node();

    node.consume(); // Consume '@'
    node.append_child(parse_attribute_list().node);
    node.append_child(child_parser().node);
    return node.build(AST_ATTRIBUTE_WRAPPER);
}

ParseResult Parser::parse_attribute_list() {
    if (stream.get()->is(TKN_LBRACKET)) {
        return parse_list(AST_ATTRIBUTE_LIST, TKN_RBRACKET, std::bind(&Parser::parse_attribute, this));
    } else {
        NodeBuilder node = build_node();
        node.append_child(parse_attribute().node);
        return node.build(AST_ATTRIBUTE_LIST);
    }
}

ParseResult Parser::parse_attribute() {
    if (stream.peek(1)->is(TKN_EQ)) {
        NodeBuilder node = build_node();
        node.append_child(consume_into_node(AST_IDENTIFIER));
        node.consume(); // Consume '='
        node.append_child(consume_into_node(AST_IDENTIFIER));
        return node.build(AST_ATTRIBUTE_VALUE);
    } else {
        return consume_into_node(AST_ATTRIBUTE_TAG);
    }
}

ParseResult Parser::parse_list(
    ASTNodeType type,
    TokenType terminator,
    const ListElementParser &element_parser,
    bool consume_terminator /* = true */
) {
    NodeBuilder node = build_node();
    node.consume(); // Consume starting token

    while (true) {
        if (stream.get()->is(terminator)) {
            if (consume_terminator) {
                node.consume();
            }

            return node.build(type);
        } else {
            ParseResult result = element_parser();
            node.append_child(result.node);

            if (!result.is_valid) {
                return {node.build(type), false};
            }
        }

        if (stream.get()->is(terminator)) {
            if (consume_terminator) {
                node.consume();
            }

            return node.build(type);
        } else if (stream.get()->is(TKN_COMMA)) {
            node.consume();
        } else {
            report_unexpected_token();
            return {node.build(type), false};
        }
    }
}

ParseResult Parser::parse_param_list(TokenType terminator /* = TKN_RPAREN */) {
    return parse_list(AST_PARAM_LIST, terminator, std::bind(&Parser::parse_param, this));
}

ParseResult Parser::parse_param() {
    NodeBuilder node = build_node();
    ASTNodeType type = AST_PARAM;

    if (stream.get()->is(TKN_AT)) {
        return parse_attribute_wrapper(std::bind(&Parser::parse_param, this));
    }

    bool is_mut = false;

    if (stream.get()->is(TKN_MUT)) {
        is_mut = true;
        node.consume(); // Consume 'mut'
    }

    if (stream.get()->is(TKN_SELF)) {
        type = is_mut ? AST_REF_MUT_PARAM : AST_REF_PARAM;
        node.append_child(consume_into_node(AST_SELF));
    } else {
        if (is_mut) {
            report_unexpected_token();
        }

        if (stream.get()->is(TKN_REF)) {
            node.consume(); // Consume 'ref'

            if (stream.get()->is(TKN_MUT)) {
                is_mut = true;
                node.consume(); // Consume 'mut'
            }

            type = is_mut ? AST_REF_MUT_PARAM : AST_REF_PARAM;
            node.append_child(consume_into_node(AST_IDENTIFIER));
            node.consume(); // Consume ':'
        } else if (stream.peek(1)->is(TKN_COLON)) {
            node.append_child(consume_into_node(AST_IDENTIFIER));
            node.consume(); // Consume ':'
        } else {
            node.append_child(create_node(AST_EMPTY, "", TextRange{0, 0}));
        }

        ParseResult result = parse_type();
        node.append_child(result.node);

        if (!result.is_valid) {
            if (stream.get()->is(TKN_RPAREN)) {
                node.consume();
            }

            return {node.build(type), false};
        }
    }

    return node.build(type);
}

ParseResult Parser::parse_return_type() {
    if (stream.get()->is(TKN_REF)) {
        NodeBuilder ref_node = build_node();
        ref_node.consume(); // Consume 'ref'

        bool is_mut = false;

        if (stream.get()->is(TKN_MUT)) {
            is_mut = true;
            ref_node.consume(); // Consume 'mut'
        }

        ParseResult result = parse_type();

        ref_node.append_child(result.node);
        return ref_node.build(is_mut ? AST_REF_MUT_RETURN : AST_REF_RETURN);
    } else {
        return parse_type();
    }
}

ParseResult Parser::check_stmt_terminator(NodeBuilder &builder, ASTNodeType type) {
    if (stream.get()->is(TKN_SEMI)) {
        builder.consume();
        return builder.build(type);
    } else if (stream.previous()->end_of_line) {
        return builder.build(type);
    } else {
        report_unexpected_token(ReportTextType::ERR_PARSE_EXPECTED_SEMI);
        return builder.build_error(type);
    }
}

Report &Parser::register_error(TextRange range) {
    mod->reports.push_back(Report(Report::Type::ERROR, SourceLocation{&file, range}));
    mod->is_valid = false;
    return mod->reports.back();
}

void Parser::report_unexpected_token() {
    report_unexpected_token(ReportTextType::ERR_PARSE_UNEXPECTED);
}

void Parser::report_unexpected_token(ReportTextType report_text_type) {
    std::string_view format_str;

    switch (report_text_type) {
        case ReportTextType::ERR_PARSE_UNEXPECTED: format_str = "unexpected token $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED: format_str = "expected $, got $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_SEMI: format_str = "expected ';' after statement"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER: format_str = "expected identifier, got $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_TYPE: format_str = "expected type, got $"; break;
    }

    Token *token = stream.get();
    register_error(token->range()).set_message(ReportText(format_str).format(token_to_str(token)).str());
}

void Parser::report_unexpected_token(ReportTextType report_text_type, std::string expected) {
    std::string_view format_str;

    switch (report_text_type) {
        case ReportTextType::ERR_PARSE_UNEXPECTED: format_str = "unexpected token $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED: format_str = "expected $, got $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_SEMI: format_str = "expected ';' after statement"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_IDENTIFIER: format_str = "expected identifier, got $"; break;
        case ReportTextType::ERR_PARSE_EXPECTED_TYPE: format_str = "expected type, got $"; break;
    }

    Token *token = stream.get();
    register_error(token->range())
        .set_message(ReportText(format_str).format(expected).format(token_to_str(token)).str());
}

std::string Parser::token_to_str(Token *token) {
    if (token->is(TKN_EOF)) return "end of file";
    else if (token->is(TKN_STRING)) return std::string{token->value};
    else return "'" + std::string{token->value} + "'";
}

void Parser::recover() {
    // Ensure forward progress.
    if (stream.get_position() == prev_recover_pos) {
        stream.consume();
    }

    while (!is_at_recover_punctuation() && !is_at_recover_keyword() && !stream.get()->is(TKN_EOF)) {
        Token *token = stream.consume();

        if (token->is(TKN_LBRACE)) {
            int depth = 1;

            while (depth > 0 && !stream.get()->is(TKN_EOF)) {
                token = stream.consume();

                if (token->is(TKN_LBRACE)) {
                    depth++;
                } else if (token->is(TKN_RBRACE)) {
                    depth--;
                }
            }
        }
    }

    if (stream.get()->is(TKN_SEMI)) {
        stream.consume();
    }

    prev_recover_pos = stream.get_position();
}

bool Parser::is_at_recover_punctuation() {
    Token *token = stream.get();
    return token->is(TKN_SEMI) || token->is(TKN_RBRACE);
}

bool Parser::is_at_recover_keyword() {
    return RECOVER_KEYWORDS.contains(stream.get()->type);
}

bool Parser::is_at_completion_point() {
    return running_completion && stream.get()->is(TKN_COMPLETION);
}

ASTNode *Parser::parse_completion_point() {
    completion_node = create_node(AST_COMPLETION_TOKEN, stream.consume());
    return completion_node;
}

ASTNode *Parser::create_dummy_block() {
    return create_node(AST_BLOCK, TextRange{stream.get()->position, stream.get()->position});
}

} // namespace lang

} // namespace banjo
