#ifndef BANJO_PARSER_H
#define BANJO_PARSER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/node_builder.hpp"
#include "banjo/parser/token_stream.hpp"
#include "banjo/reports/report.hpp"
#include "banjo/reports/report_generator.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/source_file.hpp"

#include <functional>
#include <memory>

namespace banjo::lang {

typedef std::function<ParseResult()> ListElementParser;

class Parser {

    friend class DeclParser;
    friend class StmtParser;
    friend class ExprParser;

public:
    enum class Mode {
        COMPILATION,
        FORMATTING,
    };

private:
    SourceFile &file;
    TokenStream stream;
    ReportGenerator report_generator;
    Mode mode;

    std::unique_ptr<ASTModule> mod;
    std::optional<unsigned> prev_recover_pos;
    bool skip_stmt_decl_recovery = false;

    bool running_completion = false;
    ASTNode *completion_node = nullptr;

public:
    Parser(SourceFile &file, TokenList &input, ReportManager &report_manager, Mode mode = Mode::COMPILATION);
    void enable_completion();

    std::unique_ptr<ASTModule> parse_module();

    Mode get_mode() { return mode; }
    ASTNode *get_completion_node() { return completion_node; }

private:
    ASTNode *parse_top_level_block();
    ParseResult parse_block();
    void parse_and_append_block_child(NodeBuilder &node);

    ParseResult parse_block_child();
    ASTNode *parse_expression();
    ParseResult parse_type();
    ParseResult parse_expr_or_assign();
    ParseResult parse_try();
    ParseResult parse_meta();

    ParseResult parse_attribute_wrapper(const std::function<ParseResult()> &child_parser);
    ParseResult parse_attribute_list();
    ParseResult parse_attribute();

    ParseResult parse_list(
        ASTNodeType type,
        TokenType terminator,
        const ListElementParser &element_parser,
        bool consume_terminator = true
    );

    ParseResult parse_param_list(TokenType terminator = TKN_RPAREN);
    ParseResult parse_param();
    ParseResult parse_return_type();
    ParseResult check_stmt_terminator(NodeBuilder &builder, ASTNodeType type);

    void recover();
    bool is_at_recover_keyword();

    bool is_at_completion_point();
    ASTNode *parse_completion_point();

    ASTNode *consume_into_node(ASTNodeType type) {
        ASTNode *node = mod->create_node(type, stream.consume());
        node->tokens = mod->create_token_index(stream.get_position() - 1);
        return node;
    }

    template <typename... Args>
    ASTNode *create_node(Args... args) {
        return mod->create_node(args...);
    }

    NodeBuilder build_node() { return NodeBuilder{stream, mod.get(), create_node()}; }
    ASTNode *create_dummy_block();
};

} // namespace banjo::lang

#endif
