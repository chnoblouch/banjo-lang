#ifndef PARSER_H
#define PARSER_H

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/parser/token_stream.hpp"
#include "banjo/reports/report.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/symbol/symbol_table.hpp"

#include <functional>
#include <vector>

namespace banjo {

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
    ASTBlock *parse_top_level_block();
    ParseResult parse_block(bool with_symbol_table = true);
    void parse_block_child(ASTBlock *block);

    ASTNode *parse_expression();
    ASTNode *parse_identifier();
    ParseResult parse_type();
    ParseResult parse_expr_or_assign();
    ParseResult parse_type_alias_or_explicit_type();
    AttributeList *parse_attribute_list();

    ParseResult parse_list(
        ASTNodeType type,
        TokenType terminator,
        const ListElementParser &element_parser,
        bool consume_terminator = true
    );

    ParseResult parse_param_list(TokenType terminator = TKN_RPAREN);
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

} // namespace banjo

#endif
