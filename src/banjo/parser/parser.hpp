#ifndef PARSER_H
#define PARSER_H

#include "ast/ast_block.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_node.hpp"
#include "lexer/token.hpp"
#include "parser/node_builder.hpp"
#include "parser/token_stream.hpp"
#include "reports/report.hpp"
#include "symbol/module_path.hpp"
#include "symbol/symbol_table.hpp"

#include <functional>
#include <vector>

namespace lang {

struct ParsedAST {
    ASTModule *module_;
    bool is_valid;
    std::vector<Report> reports;
};

typedef std::function<ParseResult(NodeBuilder &list_node)> ListElementParser;

class Parser {

    friend class DeclParser;
    friend class StmtParser;
    friend class ExprParser;

private:
    TokenStream stream;
    const ModulePath &module_path;
    AttributeList *current_attr_list = nullptr;
    SymbolTable *cur_symbol_table = nullptr;

    bool running_completion = false;
    TextPosition completion_point;
    ASTNode *completion_node = nullptr;

    bool is_valid = false;
    std::vector<Report> reports;

public:
    Parser(std::vector<Token> &tokens, const ModulePath &module_path);
    void enable_completion(TextPosition completion_point);

    ParsedAST parse_module();
    ASTNode *get_completion_node() { return completion_node; }

private:
    ParseResult parse_block(bool with_curly_brackets, bool with_symbol_table = true);

    ASTNode *parse_break();
    ASTNode *parse_continue();
    ParseResult parse_function_call(ASTNode *location, bool consume_semicolon);
    ASTNode *parse_return();
    ASTNode *parse_use();

    ASTNode *parse_expression();
    ParseResult parse_param_list(TokenType terminator = TKN_RPAREN);
    ASTNode *parse_generic_param_list();
    ParseResult parse_function_call_arguments();
    ASTNode *parse_identifier();
    ParseResult parse_use_tree();
    ParseResult parse_use_tree_element();
    ParseResult parse_generic_instantiation(ASTNode *template_node);

    ASTNode *parse_expr_or_assign();
    ASTNode *parse_assign(ASTNode *lhs, ASTNodeType type);

    ParseResult parse_type();

    void parse_attribute_list();

    ParseResult parse_list(ASTNodeType type, TokenType terminator, ListElementParser element_parser);
    ParseResult check_semi(ASTNode *node);

    NodeBuilder new_node();
    Report &register_error(TextRange range);

    void report_unexpected_token();
    void report_unexpected_token(ReportText::ID report_text_id);
    void report_unexpected_token(ReportText::ID report_text_id, std::string expected);
    std::string token_to_str(Token *token);

    void recover();
    bool is_at_recover_punctuation();
    bool is_at_recover_keyword();

    bool is_at_completion_point();
    ASTNode *parse_completion_point();

    ASTBlock *create_dummy_block();
};

} // namespace lang

#endif
