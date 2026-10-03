#include "assembler_lexer.hpp"
#include "banjo/utils/macros.hpp"

#include <string_view>
#include <vector>

namespace banjo::test::assembler {

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

Lexer::Lexer(std::string_view source) : source{source}, position{0} {}

TokenStream Lexer::tokenize() {
    while (position < source.size()) {
        skip_whitespace();

        if (position >= source.size()) {
            break;
        }

        token_start = position;
        char c = get();

        if (is_digit(c) || (c == '-' && position < source.size() - 1 && is_digit(source[position + 1]))) {
            read_number();
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            read_identifier();
        } else if (c == '+') {
            read_plus();
        } else if (c == '-') {
            read_minus();
        } else if (c == '*') {
            read_star();
        } else if (c == ',') {
            read_comma();
        } else if (c == ':') {
            read_colon();
        } else if (c == '!') {
            read_exclamation();
        } else if (c == '[') {
            read_lbracket();
        } else if (c == ']') {
            read_rbracket();
        } else if (c == '\n') {
            read_end_of_line();
        } else if (c == '#') {
            skip_comment();
        } else {
            // TODO
            ASSERT_UNREACHABLE;
        }
    }

    tokens.push_back(Token{.type = TokenType::END_OF_FILE, .value{}});

    return TokenStream{
        .tokens = std::move(tokens),
        .position = 0,
    };
}

void Lexer::read_identifier() {
    consume();

    while (position < source.size()) {
        char c = get();

        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || is_digit(c))) {
            break;
        }

        consume();
    }

    emit_token(TokenType::IDENTIFIER);
}

void Lexer::read_number() {
    consume();

    while (position < source.size()) {
        char c = get();

        if ((c < '0' || c > '9') && c != '.') {
            break;
        }

        consume();
    }

    emit_token(TokenType::NUMBER);
}

void Lexer::read_plus() {
    consume();
    emit_token(TokenType::PLUS);
}

void Lexer::read_minus() {
    consume();
    emit_token(TokenType::MINUS);
}

void Lexer::read_star() {
    consume();
    emit_token(TokenType::STAR);
}

void Lexer::read_comma() {
    consume();
    emit_token(TokenType::COMMA);
}

void Lexer::read_colon() {
    consume();
    emit_token(TokenType::COLON);
}

void Lexer::read_exclamation() {
    consume();
    emit_token(TokenType::EXCLAMATION);
}

void Lexer::read_lbracket() {
    consume();
    emit_token(TokenType::LBRACKET);
}

void Lexer::read_rbracket() {
    consume();
    emit_token(TokenType::RBRACKET);
}

void Lexer::read_end_of_line() {
    consume();
    emit_token(TokenType::END_OF_LINE);
}

void Lexer::skip_whitespace() {
    while (position < source.size()) {
        char c = get();

        if (c != ' ' && c != '\r' && c != '\t') {
            break;
        }

        c = consume();
    }
}

void Lexer::skip_comment() {
    while (position < source.size()) {
        char c = consume();

        if (c == '\n') {
            break;
        }
    }
}

void Lexer::emit_token(TokenType type) {
    Token token{
        .type = type,
        .value = source.substr(token_start, position - token_start),
    };

    tokens.push_back(token);
}

} // namespace banjo::test::assembler
