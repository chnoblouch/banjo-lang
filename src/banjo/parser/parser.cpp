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
    TKN_IF,
    TKN_WHILE,
    TKN_FOR,
    TKN_BREAK,
    TKN_CONTINUE,
    TKN_RETURN,
    TKN_VAR,
    TKN_FUNC,
    TKN_STRUCT,
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
    module_->append_child(parse_block(false).node); // TODO
    module_->set_range_from_children();

    return ParsedAST{
        .module_ = module_,
        .is_valid = is_valid,
        .reports = reports,
    };
}

ParseResult Parser::parse_block(bool with_curly_brackets, bool with_symbol_table /* = true */) {
    ASTBlock *block = new ASTBlock({0, 0}, cur_symbol_table);

    if (with_curly_brackets) {
        if (stream.get()->is(TKN_LBRACE)) {
            stream.consume(); // Consume '{'
        } else {
            report_unexpected_token(ReportText::ERR_PARSE_EXPECTED, "'{'");
            return {block, false};
        }
    }

    if (with_symbol_table) {
        cur_symbol_table = block->get_symbol_table();
    }

    while (stream.get()->get_type() != TKN_EOF) {
        Token *current_token = stream.get();

        if (with_curly_brackets && current_token->is(TKN_RBRACE)) {
            break;
        }

        ASTNode *node = nullptr;

        if (current_token->is(TKN_VAR)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_CONST)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_FUNC)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_IF)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_SWITCH)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_WHILE)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_FOR)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_BREAK)) node = parse_break();
        else if (current_token->is(TKN_CONTINUE)) node = parse_continue();
        else if (current_token->is(TKN_RETURN)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_PUB)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_STRUCT)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_ENUM)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_UNION)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_TYPE) && !stream.peek(1)->is(TKN_LPAREN)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_CASE)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_SELF)) node = parse_expr_or_assign();
        else if (current_token->is(TKN_NATIVE)) node = DeclParser(*this).parse();
        else if (current_token->is(TKN_USE)) node = parse_use();
        else if (current_token->is(TKN_META)) node = StmtParser(*this).parse();
        else if (current_token->is(TKN_AT)) parse_attribute_list();
        else if (current_token->is(TKN_LBRACE) || current_token->is(TKN_LBRACE)) {
            report_unexpected_token();
            stream.consume();
            continue;
        } else node = parse_expr_or_assign();

        if (node) {
            block->append_child(node);
        }
    }

    if (with_curly_brackets) {
        if (stream.get()->is(TKN_RBRACE)) {
            stream.consume(); // Consume '}'
        } else if (stream.get()->is(TKN_EOF)) {
            register_error(stream.previous()->get_range())
                .set_message(ReportText(ReportText::ID::ERR_PARSE_UNCLOSED_BLOCK).str());
            return {block, false};
        }
    }

    block->set_range_from_children();

    if (with_symbol_table) {
        cur_symbol_table = cur_symbol_table->get_parent();
    }

    return block;
}

ASTNode *Parser::parse_break() {
    TextRange range = stream.consume()->get_range();
    return check_semi(new ASTNode(AST_BREAK, range)).node;
}

ASTNode *Parser::parse_continue() {
    TextRange range = stream.consume()->get_range();
    return check_semi(new ASTNode(AST_CONTINUE, range)).node;
}

ParseResult Parser::parse_function_call(ASTNode *location, bool consume_semicolon) {
    bool is_meta_method_call = location->get_type() == AST_META_FIELD_ACCESS;
    Expr *node = is_meta_method_call ? new MetaExpr(AST_META_METHOD_CALL) : new Expr(AST_FUNCTION_CALL);
    node->append_child(location);

    ParseResult result = parse_function_call_arguments();
    node->append_child(result.node);

    if (!result.is_valid) {
        return {node, false};
    }

    node->set_range_from_children();

    if (consume_semicolon) {
        stream.consume(); // Consume ';'
    }

    return node;
}

ASTNode *Parser::parse_use() {
    NodeBuilder node = new_node();
    stream.consume(); // Consume 'use'
    node.append_child(parse_use_tree().node);
    return check_semi(node.build(AST_USE)).node;
}

ASTNode *Parser::parse_expression() {
    return ExprParser(*this).parse().node;
}

ParseResult Parser::parse_param_list(TokenType terminator /* = TKN_RPAREN */) {
    return parse_list(AST_PARAM_LIST, terminator, [this](NodeBuilder &) -> ParseResult {
        NodeBuilder param_node = new_node();

        if (stream.get()->is(TKN_AT)) {
            parse_attribute_list();
            param_node.set_attribute_list(current_attr_list);
            current_attr_list = nullptr;
        }

        if (stream.get()->is(TKN_SELF)) {
            param_node.append_child(new ASTNode(AST_SELF, "", stream.consume()->get_range()));
        } else {
            if (stream.peek(1)->is(TKN_COLON)) {
                param_node.append_child(new Identifier(stream.consume()));
                stream.consume(); // Consume ':'
            } else {
                param_node.append_child(new Identifier("", {0, 0}));
            }

            ParseResult result = parse_type();
            param_node.append_child(result.node);

            if (!result.is_valid) {
                if (stream.get()->is(TKN_RPAREN)) {
                    stream.consume();
                }

                return {param_node.build(AST_PARAM), false};
            }
        }

        return param_node.build(AST_PARAM);
    });
}

ASTNode *Parser::parse_generic_param_list() {
    return parse_list(
               AST_GENERIC_PARAM_LIST,
               TKN_RBRACKET,
               [this](NodeBuilder &) {
                   NodeBuilder parameter_node = new_node();
                   parameter_node.append_child(new Identifier(stream.consume()));

                   if (stream.get()->is(TKN_COLON)) {
                       stream.consume(); // Consume ':'

                       ParseResult result = parse_type();
                       if (result.is_valid) {
                           parameter_node.append_child(result.node);
                       } else {
                           parameter_node.append_child(new ASTNode(AST_ERROR));
                       }
                   }

                   return parameter_node.build(AST_GENERIC_PARAMETER);
               }
    ).node;
}

ParseResult Parser::parse_function_call_arguments() {
    return parse_list(AST_FUNCTION_ARGUMENT_LIST, TKN_RPAREN, [this](NodeBuilder &) {
        return ExprParser(*this, true).parse();
    });
}

ASTNode *Parser::parse_identifier() {
    return new Identifier(stream.consume());
}

ParseResult Parser::parse_use_tree() {
    ParseResult result = parse_use_tree_element();
    if (!result.is_valid) {
        return result;
    }

    ASTNode *current_node = result.node;

    while (stream.get()->is(TKN_DOT)) {
        ASTNode *dot_operator = new ASTNode(AST_DOT_OPERATOR);
        dot_operator->append_child(current_node);
        stream.consume(); // Consume dot operator

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

ParseResult Parser::parse_use_tree_element() {
    if (is_at_completion_point()) {
        return parse_completion_point();
    }

    Token *token = stream.get();
    if (token->get_type() == TKN_IDENTIFIER) {
        Identifier *identifier = new Identifier(stream.consume());

        if (!stream.get()->is(TKN_AS)) {
            return identifier;
        } else {
            stream.consume(); // Consume 'as'

            ASTNode *rebinding_node = new ASTNode(AST_USE_REBINDING);
            rebinding_node->append_child(identifier);
            rebinding_node->append_child(new Identifier(stream.consume()));
            rebinding_node->set_range_from_children();
            return rebinding_node;
        }
    } else if (token->is(TKN_LBRACE)) {
        return parse_list(AST_USE_TREE_LIST, TKN_RBRACE, [this](NodeBuilder &) { return parse_use_tree(); });
    } else {
        register_error(token->get_range())
            .set_message(ReportText(ReportText::ID::ERR_PARSE_UNEXPECTED).format(token->get_value()).str());
        return {new ASTNode(AST_ERROR), false};
    }
}

ParseResult Parser::parse_generic_instantiation(ASTNode *template_node) {
    NodeBuilder node = new_node();
    node.set_start_position(template_node->get_range().start);
    node.append_child(template_node);

    ParseResult result =
        parse_list(AST_GENERIC_ARGUMENT_LIST, TKN_RBRACKET, [this](NodeBuilder &) { return parse_type(); });
    node.append_child(result.node);

    return {node.build(new BracketExpr()), result.is_valid};
}

ASTNode *Parser::parse_expr_or_assign() {
    ParseResult result = ExprParser(*this, false).parse();
    if (!result.is_valid) {
        recover();
        return result.node;
    }
    ASTNode *node = result.node;

    switch (stream.get()->get_type()) {
        case TKN_EQ: return parse_assign(node, AST_ASSIGNMENT);
        case TKN_PLUS_EQ: return parse_assign(node, AST_ADD_ASSIGN);
        case TKN_MINUS_EQ: return parse_assign(node, AST_SUB_ASSIGN);
        case TKN_STAR_EQ: return parse_assign(node, AST_MUL_ASSIGN);
        case TKN_SLASH_EQ: return parse_assign(node, AST_DIV_ASSIGN);
        // case TKN_PERCENT_EQ: return parse_assign(node, AST_MOD_ASSIGN); // TODO
        case TKN_AND_EQ: return parse_assign(node, AST_BIT_AND_ASSIGN);
        case TKN_OR_EQ: return parse_assign(node, AST_BIT_OR_ASSIGN);
        case TKN_SHL_EQ: return parse_assign(node, AST_SHL_ASSIGN);
        case TKN_SHR_EQ: return parse_assign(node, AST_SHR_ASSIGN);
        default: return check_semi(node).node;
    }
}

ASTNode *Parser::parse_assign(ASTNode *lhs, ASTNodeType type) {
    stream.consume(); // Consume operator
    ASTNode *assign = new ASTNode(type);
    assign->append_child(lhs);
    assign->append_child(ExprParser(*this, true).parse().node);
    assign->set_range_from_children();
    return check_semi(assign).node;
}

ParseResult Parser::parse_type() {
    return ExprParser(*this).parse_type();
}

void Parser::parse_attribute_list() {
    stream.consume(); // Consume '@'

    if (!current_attr_list) {
        current_attr_list = new AttributeList();
    }

    if (stream.get()->is(TKN_LBRACKET)) {
        stream.consume(); // Consume '['

        while (!stream.get()->is(TKN_RBRACKET)) {
            std::string name = stream.consume()->get_value();
            if (stream.get()->is(TKN_EQ)) {
                stream.consume(); // Consume '='
                current_attr_list->add(Attribute(name, stream.consume()->get_value()));
            } else {
                current_attr_list->add(Attribute(name));
            }

            if (stream.get()->is(TKN_COMMA)) {
                stream.consume(); // Consume comma
            }
        }

        stream.consume(); // Consume ']'
    } else {
        std::string name = stream.consume()->get_value();
        current_attr_list->add(Attribute(name));
    }
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
