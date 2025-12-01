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
    void format_decl_block(ASTNode *node);
    void format_func_def(ASTNode *node, WhitespaceKind whitespace);
    void format_const_def(ASTNode *node, WhitespaceKind whitespace);
    void format_struct_def(ASTNode *node);

    void format_block(ASTNode *node, WhitespaceKind whitespace);

    void format_single_token_node(ASTNode *node, WhitespaceKind whitespace);
    void format_param_list(ASTNode *node, WhitespaceKind whitespace);
    void format_param(ASTNode *node, WhitespaceKind whitespace);

    void ensure_whitespace_after(unsigned token_index, WhitespaceKind whitespace);
    void ensure_no_space_after(unsigned token_index);
    void ensure_space_after(unsigned token_index);
    void ensure_indent_after(unsigned token_index, unsigned indent_addend = 0);
    void ensure_whitespace_after(unsigned token_index, std::string whitespace);
    std::string build_indent(unsigned indentation);
};

} // namespace banjo::lang

#endif
