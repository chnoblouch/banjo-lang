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

Parser::Parser(SourceFile &file, TokenList &input, ReportManager &report_manager, Mode mode /*= Mode::COMPILATION*/)
  : file{file},
    stream{input},
    report_generator{report_manager},
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
        report_generator.report_err_expected(file, *stream.get(), TKN_LBRACE);
        return node.build_error();
    }

    while (true) {
        if (stream.get()->is(TKN_RBRACE)) {
            node.consume(); // Consume '}'
            break;
        } else if (stream.get()->is(TKN_EOF)) {
            report_generator.report_err_unclosed_block(file, *stream.previous());
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
        case TKN_EQ: return StmtParser(*this).parse_assign(result.node, AST_ASSIGN_STMT);
        case TKN_PLUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_ADD_ASSIGN_STMT);
        case TKN_MINUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_SUB_ASSIGN_STMT);
        case TKN_STAR_EQ: return StmtParser(*this).parse_assign(result.node, AST_MUL_ASSIGN_STMT);
        case TKN_SLASH_EQ: return StmtParser(*this).parse_assign(result.node, AST_DIV_ASSIGN_STMT);
        case TKN_PERCENT_EQ: return StmtParser(*this).parse_assign(result.node, AST_MOD_ASSIGN_STMT);
        case TKN_AND_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_AND_ASSIGN_STMT);
        case TKN_OR_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_OR_ASSIGN_STMT);
        case TKN_CARET_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_XOR_ASSIGN_STMT);
        case TKN_SHL_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHL_ASSIGN_STMT);
        case TKN_SHR_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHR_ASSIGN_STMT);
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

    bool is_valid = true;

    while (true) {
        if (stream.get()->is(terminator)) {
            if (consume_terminator) {
                node.consume();
            }

            return {node.build(type), is_valid};
        } else {
            ParseResult result = element_parser();
            node.append_child(result.node);

            if (!result.is_valid) {
                if (stream.get()->is(TKN_COMMA)) {
                    // If a comma is encountered after a failed element parsing attempt, assume that
                    // only the current element is incomplete and continue parsing list elements.

                    node.consume();
                    is_valid = false;
                } else {
                    return {node.build(type), false};
                }
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
            report_generator.report_err_unexpected_token(file, *stream.get());
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
            report_generator.report_err_unexpected_token(file, *stream.get());
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
        report_generator.report_err_expected(file, *stream.get(), TKN_SEMI);
        return builder.build_error(type);
    }
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
