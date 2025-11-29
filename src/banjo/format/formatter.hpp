#ifndef BANJO_FORMAT_FORMATTER_H
#define BANJO_FORMAT_FORMATTER_H

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
    TokenList tokens;
    std::vector<Edit> edits;
    bool first_decl = true;
    unsigned indentation = 0;

public:
    std::vector<Edit> format(SourceFile &file);

private:
    void format_mod(ASTNode *node);
    void format_decl_block(ASTNode *node);
    void format_func_def(ASTNode *node);
    void format_const_def(ASTNode *node);
    void format_struct_def(ASTNode *node);

    void format_param_list(ASTNode *node);

    void ensure_no_space_before(unsigned token_index);
    void ensure_no_space_after(unsigned token_index);
    void ensure_space_before(unsigned token_index);
    void ensure_space_after(unsigned token_index);
    void ensure_indented(unsigned token_index);
    void ensure_whitespace_before(unsigned token_index, std::string whitespace);
    void ensure_whitespace_after(unsigned token_index, std::string whitespace);
};

} // namespace banjo::lang

#endif
