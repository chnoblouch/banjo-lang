#ifndef BANJO_TEST_UTIL_ASSEMBLER_LEXER_H
#define BANJO_TEST_UTIL_ASSEMBLER_LEXER_H

#include <string_view>
#include <vector>

namespace banjo::test::assembler {

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    PLUS,
    MINUS,
    STAR,
    COMMA,
    COLON,
    EXCLAMATION,
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
    void advance() { position += 1; }
};

class Lexer {

private:
    std::string_view source;
    std::vector<Token> tokens;

    unsigned position;
    unsigned token_start;

public:
    Lexer(std::string_view source);
    TokenStream tokenize();

private:
    void read_identifier();
    void read_number();
    void read_plus();
    void read_minus();
    void read_star();
    void read_comma();
    void read_colon();
    void read_exclamation();
    void read_lbracket();
    void read_rbracket();
    void read_end_of_line();
    void skip_whitespace();
    void skip_comment();

    void emit_token(TokenType type);

    char consume() { return source[position++]; }
    char get() { return source[position]; }
};

} // namespace banjo::test::assembler

#endif
