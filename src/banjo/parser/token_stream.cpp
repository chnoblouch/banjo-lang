#include "token_stream.hpp"

#include "banjo/lexer/token.hpp"
#include "banjo/lexer/token_list.hpp"

namespace banjo::lang {

TokenStream::TokenStream(TokenList &input) : input{input} {}

Token *TokenStream::get() {
    return &input.tokens[position];
}

Token *TokenStream::consume() {
    Token *token = &input.tokens[position];
    position++;
    return token;
}

Token *TokenStream::peek(unsigned offset) {
    return &input.tokens[position + offset];
}

Token *TokenStream::previous() {
    return position > 0 ? &input.tokens[position - 1] : &input.tokens.back();
}

void TokenStream::seek(unsigned position) {
    this->position = position;
}

void TokenStream::split_current() {
    Token *token = &input.tokens[position];

    switch (token->type) {
        case TKN_OR_OR:
            input.tokens[position] = Token{TKN_OR, "", token->position};
            input.tokens.insert(input.tokens.begin() + position, Token{TKN_OR, "", token->position + 1});
            break;
        default: break;
    }
}

} // namespace banjo::lang
