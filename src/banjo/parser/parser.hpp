#ifndef PARSER_H
#define PARSER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/parser/token_stream.hpp"
#include "banjo/reports/report.hpp"
#include "banjo/source/module_path.hpp"

#include <functional>
#include <memory>
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

public:
    enum class ReportTextType {
        ERR_PARSE_UNEXPECTED,
        ERR_PARSE_EXPECTED,
        ERR_PARSE_EXPECTED_SEMI,
        ERR_PARSE_EXPECTED_IDENTIFIER,
        ERR_PARSE_EXPECTED_TYPE,
    };

private:
    TokenStream stream;
    const ModulePath &module_path;
    std::unique_ptr<AttributeList> current_attr_list = nullptr;

    bool running_completion = false;
    ASTNode *completion_node = nullptr;

    bool is_valid = false;
    std::vector<Report> reports;

public:
    Parser(std::vector<Token> &tokens, const ModulePath &module_path);
    void enable_completion();

    ParsedAST parse_module();
    ASTNode *get_completion_node() { return completion_node; }

private:
    ASTNode *parse_top_level_block();
    ParseResult parse_block();
    void parse_block_child(ASTNode *block);

    ASTNode *parse_expression();
    ParseResult parse_type();
    ParseResult parse_expr_or_assign();
    ParseResult parse_type_alias_or_explicit_type();
    ParseResult parse_meta();
    std::unique_ptr<AttributeList> parse_attribute_list();

    ParseResult parse_list(
        ASTNodeType type,
        TokenType terminator,
        const ListElementParser &element_parser,
        bool consume_terminator = true
    );

    ParseResult parse_param_list(TokenType terminator = TKN_RPAREN);
    ParseResult check_stmt_terminator(ASTNode *node);

    NodeBuilder new_node();
    Report &register_error(TextRange range);

    void report_unexpected_token();
    void report_unexpected_token(ReportTextType report_text_type);
    void report_unexpected_token(ReportTextType report_text_type, std::string expected);
    std::string token_to_str(Token *token);

    void recover();
    bool is_at_recover_punctuation();
    bool is_at_recover_keyword();

    bool is_at_completion_point();
    ASTNode *parse_completion_point();

    ASTNode *create_dummy_block();
};

} // namespace lang

} // namespace banjo

#endif
