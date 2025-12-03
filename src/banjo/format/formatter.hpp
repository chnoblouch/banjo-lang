#ifndef BANJO_FORMAT_FORMATTER_H
#define BANJO_FORMAT_FORMATTER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/source/text_range.hpp"

#include <string>
#include <vector>

namespace banjo::lang {

class Formatter {

public:
    struct Edit {
        TextRange range;
        std::string replacement;
    };

private:
    enum class WhitespaceKind {
        NONE,
        SPACE,
        INDENT,
        INDENT_FIRST,
        INDENT_OUT,
        INDENT_EMPTY_LINE,
    };

    TokenList tokens;
    std::vector<Edit> edits;
    unsigned indentation = 0;

public:
    std::vector<Edit> format(SourceFile &file);
    void format_in_place(SourceFile &file);

private:
    void format_node(ASTNode *node, WhitespaceKind whitespace);

    void format_mod(ASTNode *node);
    void format_func_def(ASTNode *node, WhitespaceKind whitespace);
    void format_const_def(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_def(ASTNode *node, WhitespaceKind whitespace);

    void format_block(ASTNode *node, WhitespaceKind whitespace);
    void format_expr_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_var_stmt(ASTNode *node, WhitespaceKind whitespace);
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

    void format_struct_literal(ASTNode *node, WhitespaceKind whitespace);
    void format_typeless_struct_literal(ASTNode *node, WhitespaceKind whitespace);
    void format_binary_expr(ASTNode *node, WhitespaceKind whitespace, bool spaces_between = true);
    void format_unary_expr(ASTNode *node, WhitespaceKind whitespace);
    void format_call_or_bracket_expr(ASTNode *node, WhitespaceKind whitespace);
    void format_static_array_type(ASTNode *node, WhitespaceKind whitespace);

    void format_param(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_literal_entry(ASTNode *node, WhitespaceKind whitespace);

    void format_keyword_stmt(ASTNode *node, WhitespaceKind whitespace);
    void format_single_token_node(ASTNode *node, WhitespaceKind whitespace);
    void format_list(ASTNode *node, WhitespaceKind whitespace, bool enclosing_spaces = false);
    void format_before_terminator(ASTNode *parent, ASTNode *child, WhitespaceKind whitespace, unsigned semi_index);

    void ensure_whitespace_after(unsigned token_index, WhitespaceKind whitespace);
    void ensure_no_space_after(unsigned token_index);
    void ensure_space_after(unsigned token_index);
    void ensure_indent_after(unsigned token_index, WhitespaceKind whitespace);
    void ensure_indent_after(unsigned token_index, int indent_addend = 0);
    void ensure_whitespace_after(unsigned token_index, std::string whitespace);
    std::string build_indent(unsigned indentation);
};

} // namespace banjo::lang

#endif
