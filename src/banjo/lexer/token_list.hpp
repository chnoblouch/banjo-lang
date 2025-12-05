#ifndef BANJO_LEXER_TOKEN_STREAM_H
#define BANJO_LEXER_TOKEN_STREAM_H

#include "banjo/lexer/token.hpp"

#include <span>
#include <vector>

namespace banjo::lang {

struct ASTNode;

struct TokenList {
    static constexpr unsigned EOF_ZONE_SIZE = 1;

    struct Span {
        unsigned first;
        unsigned count;
    };

    std::vector<Token> tokens;
    std::vector<Token> attached_tokens;
    std::vector<Span> attachments;

    std::span<Token> get_attached_tokens(unsigned token_index);
    unsigned last_token_index(ASTNode *node);
};

} // namespace banjo::lang

#endif
