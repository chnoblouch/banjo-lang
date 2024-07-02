#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include "banjo/lexer/token.hpp"
#include <vector>

namespace banjo {

namespace lang {

class TokenStream {

private:
    std::vector<Token> &tokens;
    std::vector<Token>::size_type position;
    Token eof_token{TKN_EOF, "", 0};

public:
    TokenStream(std::vector<Token> &tokens);

    Token *get();
    Token *consume();
    Token *peek(std::vector<Token>::size_type offset);
    Token *previous();
    void seek(std::vector<Token>::size_type position);
    void split_current();
};

} // namespace lang

} // namespace banjo

#endif
