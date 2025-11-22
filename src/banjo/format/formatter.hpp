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

public:
    std::vector<Edit> format(SourceFile &file);

private:
    void format_mod(ASTNode *node);
    void format_decl_block(ASTNode *node);
    void format_const(ASTNode *node);

    void ensure_no_space_after(unsigned token_index);
    void ensure_space_after(unsigned token_index);
};

} // namespace banjo::lang

#endif
