#include "token_stream.hpp"

namespace banjo {

namespace lang {

TokenStream::TokenStream(std::vector<Token> &tokens) : tokens(tokens) {}

Token *TokenStream::get() {
    if (position >= tokens.size()) {
        return &eof_token;
    }

    return &tokens[position];
}

Token *TokenStream::consume() {
    if (position >= tokens.size()) {
        return &eof_token;
    }

    Token *token = &tokens[position];
    position++;
    return token;
}

Token *TokenStream::peek(std::vector<Token>::size_type offset) {
    if (position + offset >= tokens.size()) {
        return &eof_token;
    }

    return &tokens[position + offset];
}

Token *TokenStream::previous() {
    return position > 0 ? &tokens[position - 1] : &eof_token;
}

void TokenStream::seek(std::vector<Token>::size_type position) {
    this->position = position;
}

void TokenStream::split_current() {
    Token *token = &tokens[position];

    switch (token->type) {
        case TKN_OR_OR:
            tokens[position] = Token(TKN_OR, "", token->position);
            tokens.insert(tokens.begin() + position, Token(TKN_OR, "", token->position + 1));
            break;
        default: break;
    }
}

} // namespace lang

} // namespace banjo
