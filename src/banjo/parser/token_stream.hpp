#ifndef BANJO_PARSER_TOKEN_STREAM_H
#define BANJO_PARSER_TOKEN_STREAM_H

#include "banjo/lexer/token.hpp"
#include "banjo/lexer/token_list.hpp"

namespace banjo {

class TokenStream {

private:
    TokenList &input;
    unsigned position;

public:
    TokenStream(TokenList &input);

    unsigned get_position() { return position; }
    Token *get();
    Token *consume();
    Token *peek(unsigned offset);
    Token *previous();
    void seek(unsigned position);
};

} // namespace banjo

#endif
