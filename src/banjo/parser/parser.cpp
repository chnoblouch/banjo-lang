#include "parser.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/decl_parser.hpp"
#include "banjo/parser/expr_parser.hpp"
#include "banjo/parser/stmt_parser.hpp"
#include "banjo/utils/timing.hpp"

#include <memory>
#include <unordered_set>

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

Parser::Parser(std::vector<Token> &tokens, const ModulePath &module_path, Mode mode /*= Mode::COMPILATION*/)
  : stream(tokens),
    module_path(module_path),
    mode(mode) {}

void Parser::enable_completion() {
    running_completion = true;
}

ParsedAST Parser::parse_module() {
    PROFILE_SCOPE("parser");

    stream.seek(0);
    is_valid = true;

    cur_mod = new ASTModule(module_path);
    cur_mod->append_child(parse_top_level_block());
    cur_mod->set_range_from_children();

    return ParsedAST{
        .module_ = cur_mod,
        .is_valid = is_valid,
        .reports = reports,
    };
}

ASTNode *Parser::parse_top_level_block() {
    ASTNode *block = create_node(AST_BLOCK, TextRange{0, 0});

    while (!stream.get()->is(TKN_EOF)) {
        parse_and_append_block_child(block);
    }

    block->set_range_from_children();
    return block;
}

ParseResult Parser::parse_block() {
    ASTNode *block = create_node(AST_BLOCK, TextRange{0, 0});

    if (stream.get()->is(TKN_LBRACE)) {
        stream.consume(); // Consume '{'
    } else {
        report_unexpected_token(Parser::ReportTextType::ERR_PARSE_EXPECTED, "'{'");
        return {block, false};
    }

    while (true) {
        if (stream.get()->is(TKN_RBRACE)) {
            stream.consume(); // Consume '}'
            break;
        } else if (stream.get()->is(TKN_EOF)) {
            register_error(stream.previous()->range()).set_message(ReportText("file ends with unclosed block").str());
            return {block, false};
        } else {
            parse_and_append_block_child(block);
        }
    }

    block->set_range_from_children();
    return block;
}

void Parser::parse_and_append_block_child(ASTNode *block) {
    ParseResult result = parse_block_child();
    block->append_child(result.node);

    if (!result.is_valid) {
        recover();
    }
}

ParseResult Parser::parse_block_child() {
    if (mode == Mode::FORMATTING && stream.get()->after_empty_line) {
        return create_node(AST_EMPTY_LINE);
    }

    switch (stream.get()->type) {
        case TKN_VAR: return StmtParser(*this).parse_var();
        case TKN_REF: return StmtParser(*this).parse_ref();
        case TKN_CONST: return DeclParser(*this).parse_const();
        case TKN_FUNC: return DeclParser(*this).parse_func(nullptr);
        case TKN_IF: return StmtParser(*this).parse_if_chain();
        case TKN_SWITCH: return StmtParser(*this).parse_switch();
        case TKN_TRY: return StmtParser(*this).parse_try();
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
        case TKN_TYPE: return parse_type_alias_or_explicit_type();
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
    ParseResult result = ExprParser(*this, false).parse();
    if (!result.is_valid) {
        return {result.node, false};
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
        default: return check_stmt_terminator(result.node);
    }
}

ParseResult Parser::parse_type_alias_or_explicit_type() {
    if (stream.peek(1)->is(TKN_LPAREN)) {
        return parse_expr_or_assign();
    } else {
        return DeclParser(*this).parse_type_alias();
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

    stream.consume(); // Consume '@'
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
        node.append_child(create_node(AST_IDENTIFIER, stream.consume()));
        stream.consume(); // Consume '='
        node.append_child(create_node(AST_IDENTIFIER, stream.consume()));
        return node.build(AST_ATTRIBUTE_VALUE);
    } else {
        return create_node(AST_ATTRIBUTE_TAG, stream.consume());
    }
}

ParseResult Parser::parse_list(
    ASTNodeType type,
    TokenType terminator,
    const ListElementParser &element_parser,
    bool consume_terminator /* = true */
) {
    NodeBuilder node = build_node();
    stream.consume(); // Consume starting token

    while (true) {
        if (stream.get()->is(terminator)) {
            if (consume_terminator) {
                stream.consume();
            }

            ASTNode *result = node.build(type);
            if (mode == Mode::FORMATTING && stream.peek(-2)->is(TKN_COMMA)) {
                result->flags.trailing_comma = true;
            }

            return result;
        } else {
            ParseResult result = element_parser();
            node.append_child(result.node);

            if (!result.is_valid) {
                return {node.build(type), false};
            }
        }

        if (stream.get()->is(terminator)) {
            if (consume_terminator) {
                stream.consume();
            }

            return node.build(type);
        } else if (stream.get()->is(TKN_COMMA)) {
            stream.consume();
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
        stream.consume(); // Consume 'mut'
    }

    if (stream.get()->is(TKN_SELF)) {
        type = is_mut ? AST_REF_MUT_PARAM : AST_REF_PARAM;
        node.append_child(create_node(AST_SELF, "", stream.consume()->range()));
    } else {
        if (is_mut) {
            report_unexpected_token();
        }

        if (stream.get()->is(TKN_REF)) {
            stream.consume(); // Consume 'ref'

            if (stream.get()->is(TKN_MUT)) {
                is_mut = true;
                stream.consume(); // Consume 'mut'
            }

            type = is_mut ? AST_REF_MUT_PARAM : AST_REF_PARAM;
            node.append_child(create_node(AST_IDENTIFIER, stream.consume()));
            stream.consume(); // Consume ':'
        } else if (stream.peek(1)->is(TKN_COLON)) {
            node.append_child(create_node(AST_IDENTIFIER, stream.consume()));
            stream.consume(); // Consume ':'
        } else {
            node.append_child(create_node(AST_IDENTIFIER, "", TextRange{0, 0}));
        }

        ParseResult result = parse_type();
        node.append_child(result.node);

        if (!result.is_valid) {
            if (stream.get()->is(TKN_RPAREN)) {
                stream.consume();
            }

            return {node.build(type), false};
        }
    }

    return node.build(type);
}

ParseResult Parser::parse_return_type() {
    if (stream.get()->is(TKN_REF)) {
        stream.consume(); // Consume 'ref'

        bool is_mut = false;

        if (stream.get()->is(TKN_MUT)) {
            is_mut = true;
            stream.consume(); // Consume 'mut'
        }

        ParseResult result = parse_type();

        ASTNode *ref_node = create_node(is_mut ? AST_REF_MUT_RETURN : AST_REF_RETURN);
        ref_node->append_child(result.node);
        ref_node->set_range_from_children();
        return ref_node;
    } else {
        return parse_type();
    }
}

ParseResult Parser::check_stmt_terminator(ASTNode *node) {
    if (stream.get()->is(TKN_SEMI)) {
        stream.consume();
        return {node, true};
    } else if (stream.previous()->end_of_line) {
        return {node, true};
    } else {
        report_unexpected_token(ReportTextType::ERR_PARSE_EXPECTED_SEMI);
        return {node, false};
    }
}

Report &Parser::register_error(TextRange range) {
    reports.push_back(Report(Report::Type::ERROR, SourceLocation{module_path, range}));
    is_valid = false;
    return reports.back();
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

    if (stream.get()->is(TKN_SEMI) || stream.get()->is(TKN_RBRACE)) {
        stream.consume();
    }
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
