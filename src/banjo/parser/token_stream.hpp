#ifndef BANJO_PARSER_TOKEN_STREAM_H
#define BANJO_PARSER_TOKEN_STREAM_H

#include "banjo/lexer/token.hpp"

#include <vector>

namespace banjo {

namespace lang {

class TokenStream {

private:
    TokenList &input;
    unsigned position;
    Token eof_token{TKN_EOF, "", 0};

public:
    TokenStream(TokenList &input);

    unsigned get_position() { return position; }
    Token *get();
    Token *consume();
    Token *peek(unsigned offset);
    Token *previous();
    void seek(unsigned position);
    void split_current();
    std::span<Token> previous_attached_tokens();
};

} // namespace lang

} // namespace banjo

#endif
