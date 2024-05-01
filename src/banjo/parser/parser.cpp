#include "parser.hpp"

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "lexer/token.hpp"
#include "parser/decl_parser.hpp"
#include "parser/expr_parser.hpp"
#include "parser/stmt_parser.hpp"
#include "symbol/symbol_table.hpp"
#include "utils/timing.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_set>

namespace lang {

const std::unordered_set<TokenType> PRIMITIVE_DATA_TYPES{
    TKN_I8,
    TKN_I16,
    TKN_I32,
    TKN_I64,
    TKN_U8,
    TKN_U16,
    TKN_U32,
    TKN_U64,
    TKN_F32,
    TKN_F64,
    TKN_USIZE,
    TKN_BOOL,
    TKN_ADDR,
    TKN_VOID
};

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

Parser::Parser(std::vector<Token> &tokens, const ModulePath &module_path) : stream(tokens), module_path(module_path) {}

void Parser::enable_completion(TextPosition completion_point) {
    running_completion = true;
    this->completion_point = completion_point;
}

ParsedAST Parser::parse_module() {
    PROFILE_SCOPE("parser");

    stream.seek(0);
    is_valid = true;

    ASTModule *module_ = new ASTModule(module_path);
    module_->append_child(parse_top_level_block());
    module_->set_range_from_children();

    return ParsedAST{
        .module_ = module_,
        .is_valid = is_valid,
        .reports = reports,
    };
}

ASTBlock *Parser::parse_top_level_block() {
    ASTBlock *block = new ASTBlock({0, 0}, cur_symbol_table);
    cur_symbol_table = block->get_symbol_table();

    while (!stream.get()->is(TKN_EOF)) {
        parse_block_child(block);
    }

    block->set_range_from_children();
    cur_symbol_table = cur_symbol_table->get_parent();
    return block;
}

ParseResult Parser::parse_block(bool with_symbol_table /* = true */) {
    ASTBlock *block = new ASTBlock({0, 0}, cur_symbol_table);

    if (stream.get()->is(TKN_LBRACE)) {
        stream.consume(); // Consume '{'
    } else {
        report_unexpected_token(ReportText::ERR_PARSE_EXPECTED, "'{'");
        return {block, false};
    }

    if (with_symbol_table) {
        cur_symbol_table = block->get_symbol_table();
    }

    while (true) {
        if (stream.get()->is(TKN_RBRACE)) {
            stream.consume(); // Consume '}'
            break;
        } else if (stream.get()->is(TKN_EOF)) {
            register_error(stream.previous()->get_range())
                .set_message(ReportText(ReportText::ID::ERR_PARSE_UNCLOSED_BLOCK).str());
            return {block, false};
        } else {
            parse_block_child(block);
        }
    }

    block->set_range_from_children();

    if (with_symbol_table) {
        cur_symbol_table = cur_symbol_table->get_parent();
    }

    return block;
}

void Parser::parse_block_child(ASTBlock *block) {
    ParseResult result;

    switch (stream.get()->get_type()) {
        case TKN_VAR: result = StmtParser(*this).parse_var(); break;
        case TKN_CONST: result = DeclParser(*this).parse_const(); break;
        case TKN_FUNC: result = DeclParser(*this).parse_func(nullptr); break;
        case TKN_IF: result = StmtParser(*this).parse_if_chain(); break;
        case TKN_SWITCH: result = StmtParser(*this).parse_switch(); break;
        case TKN_WHILE: result = StmtParser(*this).parse_while(); break;
        case TKN_FOR: result = StmtParser(*this).parse_for(); break;
        case TKN_BREAK: result = StmtParser(*this).parse_break(); break;
        case TKN_CONTINUE: result = StmtParser(*this).parse_continue(); break;
        case TKN_RETURN: result = StmtParser(*this).parse_return(); break;
        case TKN_PUB: result = DeclParser(*this).parse_pub(); break;
        case TKN_STRUCT: result = DeclParser(*this).parse_struct(); break;
        case TKN_ENUM: result = DeclParser(*this).parse_enum(); break;
        case TKN_UNION: result = DeclParser(*this).parse_union(); break;
        case TKN_TYPE: result = parse_type_alias_or_explicit_type(); break;
        case TKN_CASE: result = DeclParser(*this).parse_union_case(); break;
        case TKN_SELF: result = parse_expr_or_assign(); break;
        case TKN_NATIVE: result = DeclParser(*this).parse_native(); break;
        case TKN_USE: result = DeclParser(*this).parse_use(); break;
        case TKN_META: result = StmtParser(*this).parse_meta_stmt(); break;
        case TKN_AT: current_attr_list = parse_attribute_list(); return;
        default: result = parse_expr_or_assign();
    }

    if (result.is_valid) {
        block->append_child(result.node);
    } else {
        recover();
    }
}

ASTNode *Parser::parse_expression() {
    return ExprParser(*this).parse().node;
}

ASTNode *Parser::parse_identifier() {
    return new Identifier(stream.consume());
}

ParseResult Parser::parse_expr_or_assign() {
    ParseResult result = ExprParser(*this, false).parse();
    if (!result.is_valid) {
        return {result.node, false};
    }

    switch (stream.get()->get_type()) {
        case TKN_EQ: return StmtParser(*this).parse_assign(result.node, AST_ASSIGNMENT);
        case TKN_PLUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_ADD_ASSIGN);
        case TKN_MINUS_EQ: return StmtParser(*this).parse_assign(result.node, AST_SUB_ASSIGN);
        case TKN_STAR_EQ: return StmtParser(*this).parse_assign(result.node, AST_MUL_ASSIGN);
        case TKN_SLASH_EQ: return StmtParser(*this).parse_assign(result.node, AST_DIV_ASSIGN);
        // case TKN_PERCENT_EQ: return StmtParser(*this).parse_assign(node, AST_MOD_ASSIGN); // TODO
        case TKN_AND_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_AND_ASSIGN);
        case TKN_OR_EQ: return StmtParser(*this).parse_assign(result.node, AST_BIT_OR_ASSIGN);
        case TKN_SHL_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHL_ASSIGN);
        case TKN_SHR_EQ: return StmtParser(*this).parse_assign(result.node, AST_SHR_ASSIGN);
        default: return check_semi(result.node);
    }
}

ParseResult Parser::parse_type_alias_or_explicit_type() {
    if (stream.peek(1)->is(TKN_LPAREN)) {
        return ExprParser(*this).parse();
    } else {
        return DeclParser(*this).parse_type_alias();
    }
}

ParseResult Parser::parse_type() {
    return ExprParser(*this).parse_type();
}

AttributeList *Parser::parse_attribute_list() {
    stream.consume(); // Consume '@'

    AttributeList *attr_list = new AttributeList();

    if (stream.get()->is(TKN_LBRACKET)) {
        stream.consume(); // Consume '['

        while (!stream.get()->is(TKN_RBRACKET)) {
            std::string name = stream.consume()->get_value();
            if (stream.get()->is(TKN_EQ)) {
                stream.consume(); // Consume '='
                attr_list->add(Attribute(name, stream.consume()->get_value()));
            } else {
                attr_list->add(Attribute(name));
            }

            if (stream.get()->is(TKN_COMMA)) {
                stream.consume(); // Consume comma
            }
        }

        stream.consume(); // Consume ']'
    } else {
        std::string name = stream.consume()->get_value();
        attr_list->add(Attribute(name));
    }

    return attr_list;
}

ParseResult Parser::parse_list(ASTNodeType type, TokenType terminator, ListElementParser element_parser) {
    NodeBuilder node = new_node();
    stream.consume(); // Consume starting token

    while (true) {
        if (stream.get()->is(terminator)) {
            stream.consume();
            return node.build(type);
        } else {
            ParseResult result = element_parser(node);
            node.append_child(result.node);

            if (!result.is_valid) {
                return {node.build(type), false};
            }
        }

        if (stream.get()->is(terminator)) {
            stream.consume();
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
    return parse_list(AST_PARAM_LIST, terminator, [this](NodeBuilder &) -> ParseResult {
        NodeBuilder node = new_node();

        if (stream.get()->is(TKN_AT)) {
            parse_attribute_list();
            node.set_attribute_list(current_attr_list);
            current_attr_list = nullptr;
        }

        if (stream.get()->is(TKN_SELF)) {
            node.append_child(new ASTNode(AST_SELF, "", stream.consume()->get_range()));
        } else {
            if (stream.peek(1)->is(TKN_COLON)) {
                node.append_child(new Identifier(stream.consume()));
                stream.consume(); // Consume ':'
            } else {
                node.append_child(new Identifier("", {0, 0}));
            }

            ParseResult result = parse_type();
            node.append_child(result.node);

            if (!result.is_valid) {
                if (stream.get()->is(TKN_RPAREN)) {
                    stream.consume();
                }

                return {node.build(AST_PARAM), false};
            }
        }

        return node.build(AST_PARAM);
    });
}

ParseResult Parser::check_semi(ASTNode *node) {
    if (stream.get()->is(TKN_SEMI)) {
        stream.consume(); // Consume ';'
        return {node, true};
    } else {
        report_unexpected_token(ReportText::ID::ERR_PARSE_EXPECTED_SEMI);
        return {node, false};
    }
}

NodeBuilder Parser::new_node() {
    return NodeBuilder(stream);
}

Report &Parser::register_error(TextRange range) {
    reports.push_back(Report(Report::Type::ERROR, SourceLocation{module_path, range}));
    is_valid = false;
    return reports.back();
}

void Parser::report_unexpected_token() {
    report_unexpected_token(ReportText::ID::ERR_PARSE_UNEXPECTED);
}

void Parser::report_unexpected_token(ReportText::ID report_text_id) {
    Token *token = stream.get();
    register_error(token->get_range()).set_message(ReportText(report_text_id).format(token_to_str(token)).str());
}

void Parser::report_unexpected_token(ReportText::ID report_text_id, std::string expected) {
    Token *token = stream.get();
    register_error(token->get_range())
        .set_message(ReportText(report_text_id).format(expected).format(token_to_str(token)).str());
}

std::string Parser::token_to_str(Token *token) {
    if (token->get_type() == TKN_EOF) return "end of file";
    else if (token->get_type() == TKN_STRING) return token->get_value();
    else return "'" + token->get_value() + "'";
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
    return RECOVER_KEYWORDS.count(stream.get()->get_type());
}

bool Parser::is_at_completion_point() {
    return running_completion && stream.get()->get_type() == TKN_COMPLETION;
}

ASTNode *Parser::parse_completion_point() {
    completion_node = new Identifier(stream.consume());
    return completion_node;
}

ASTBlock *Parser::create_dummy_block() {
    return new ASTBlock({stream.get()->get_position(), stream.get()->get_position()}, cur_symbol_table);
}

} // namespace lang
