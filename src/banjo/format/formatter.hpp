#ifndef BANJO_FORMAT_FORMATTER_H
#define BANJO_FORMAT_FORMATTER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/edit.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/source/text_range.hpp"

#include <functional>
#include <string>

namespace banjo::lang {

class Formatter {

private:
    enum class WhitespaceKind {
        NONE,
        SPACE,
        INDENT,
        INDENT_ALLOW_EMPTY_LINE,
        INDENT_OUT,
        INDENT_EMPTY_LINE,
    };

    ReportManager &report_manager;
    SourceFile &file;
    EditList edits;

    TokenList tokens;
    unsigned indentation = 0;
    bool global_scope = true;

public:
    Formatter(ReportManager &report_manager, SourceFile &file);
    EditList format();

private:
    void format_node(ASTNode *node, WhitespaceKind whitespace);

    void format_mod(ASTNode *node);
    void format_func_def(ASTNode *node, WhitespaceKind whitespace);
    void format_generic_func_def(ASTNode *node, WhitespaceKind whitespace);
    void format_func_decl(ASTNode *node, WhitespaceKind whitespace);
    void format_const_def(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_def(ASTNode *node, WhitespaceKind whitespace);
    void format_generic_struct_def(ASTNode *node, WhitespaceKind whitespace);
    void format_enum_def(ASTNode *node, WhitespaceKind whitespace);
    void format_enum_variant(ASTNode *node, WhitespaceKind whitespace);
    void format_union_def(ASTNode *node, WhitespaceKind whitespace);
    void format_union_case(ASTNode *node, WhitespaceKind whitespace);
    void format_proto_def(ASTNode *node, WhitespaceKind whitespace);
    void format_type_alias(ASTNode *node, WhitespaceKind whitespace);
    void format_use_decl(ASTNode *node, WhitespaceKind whitespace);
    void format_use_rebind(ASTNode *node, WhitespaceKind whitespace);
    void format_use_list(ASTNode *node, WhitespaceKind whitespace);

    void format_block(ASTNode *node, WhitespaceKind whitespace);
    void format_expr_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_var_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_var_decl(ASTNode *node, WhitespaceKind whitespace);
    void format_typeless_var_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_assign_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_return_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_if_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_if_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_else_if_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_else_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_switch_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_switch_case_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_try_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_try_success_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_try_except_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_try_else_branch(ASTNode *node, WhitespaceKind whitespace);
    void format_while_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_for_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_meta_if_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_meta_for_stmt(ASTNode *node, WhitespaceKind whitespace);

    void format_paren_expr(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_literal(ASTNode *node, WhitespaceKind whitespace);
    void format_typeless_struct_literal(ASTNode *node, WhitespaceKind whitespace);
    void format_map_literal_entry(ASTNode *node, WhitespaceKind whitespace);
    void format_closure_literal(ASTNode *node, WhitespaceKind whitespace);
    void format_binary_expr(ASTNode *node, WhitespaceKind whitespace, bool spaces_between = true);
    void format_unary_expr(ASTNode *node, WhitespaceKind whitespace, bool space_between = false);
    void format_call_or_bracket_expr(ASTNode *node, WhitespaceKind whitespace);
    void format_static_array_type(ASTNode *node, WhitespaceKind whitespace);
    void format_func_type(ASTNode *node, WhitespaceKind whitespace);
    void format_closure_type(ASTNode *node, WhitespaceKind whitespace);
    void format_meta_expr(ASTNode *node, WhitespaceKind whitespace);

    void format_param_list(ASTNode *node, WhitespaceKind whitespace);
    void format_param(ASTNode *node, WhitespaceKind whitespace);
    void format_ref_return(ASTNode *node, WhitespaceKind whitespace);
    void format_generic_param(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_literal_entry(ASTNode *node, WhitespaceKind whitespace);
    void format_impl_list(ASTNode *node, WhitespaceKind whitespace);
    void format_qualifier_list(ASTNode *node, WhitespaceKind whitespace);
    void format_attribute_wrapper(ASTNode *node, WhitespaceKind whitespace);
    void format_attribute_list(ASTNode *node, WhitespaceKind whitespace);
    void format_attribute_value(ASTNode *node, WhitespaceKind whitespace);

    void format_keyword_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_single_token_node(ASTNode *node, WhitespaceKind whitespace);
    void format_list(ASTNode *node, WhitespaceKind whitespace, bool enclosing_spaces = false);

    void format_list(
        ASTNode *node,
        WhitespaceKind whitespace,
        bool enclosing_spaces,
        const std::function<void(ASTNode *, WhitespaceKind whitespace)> &child_formatter
    );

    void format_before_terminator(ASTNode *parent, ASTNode *child, WhitespaceKind whitespace, unsigned semi_index);

    void ensure_no_space_after(unsigned token_index);
    void ensure_space_after(unsigned token_index);
    void ensure_whitespace_after(unsigned token_index, WhitespaceKind whitespace);
    void format_comments(std::span<Token> &attached_tokens, WhitespaceKind whitespace);
    void format_comment_text(Token &comment_token);

    std::string whitespace_as_string(WhitespaceKind whitespace);
    std::string build_indent(unsigned indentation);
    bool followed_by_comment(unsigned token_index);

    static bool compare(ASTNode *a, ASTNode *b);
    static std::string_view ordering_string(ASTNode *node);
    static unsigned ordering_value(char c);
};

} // namespace banjo::lang

#endif
