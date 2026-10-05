#ifndef BANJO_UTILS_GENERIC_LEXER_H
#define BANJO_UTILS_GENERIC_LEXER_H

#include <string_view>
#include <vector>

namespace banjo::utils {

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    PLUS,
    MINUS,
    STAR,
    PERCENT,
    EQUALS,
    COMMA,
    COLON,
    EXCLAMATION,
    AT,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    END_OF_LINE,
    END_OF_FILE,
};

struct Token {
    TokenType type;
    std::string_view value;
};

struct TokenStream {
    std::vector<Token> tokens;
    unsigned position;

    Token &get() { return tokens[position]; }
    Token &next() { return tokens[position + 1]; }
    void advance() { position += 1; }
};

class GenericLexer {

private:
    std::string_view source;
    std::vector<Token> tokens;

    unsigned position;
    unsigned token_start;

public:
    GenericLexer(std::string_view source);
    TokenStream tokenize();

private:
    void read_identifier();
    void read_number();
    void read_punctuation(TokenType type);
    void read_end_of_line();
    void skip_whitespace();
    void skip_comment();

    void emit_token(TokenType type);

    char consume() { return source[position++]; }
    char get() { return source[position]; }
};

} // namespace banjo::utils

#endif
