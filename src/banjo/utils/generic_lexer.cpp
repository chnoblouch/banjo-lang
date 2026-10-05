#include "generic_lexer.hpp"

#include "banjo/utils/macros.hpp"

#include <string_view>
#include <vector>

namespace banjo::utils {

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

GenericLexer::GenericLexer(std::string_view source) : source{source}, position{0} {}

TokenStream GenericLexer::tokenize() {
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
            read_punctuation(TokenType::PLUS);
        } else if (c == '-') {
            read_punctuation(TokenType::MINUS);
        } else if (c == '*') {
            read_punctuation(TokenType::STAR);
        } else if (c == '%') {
            read_punctuation(TokenType::PERCENT);
        } else if (c == '=') {
            read_punctuation(TokenType::EQUALS);
        } else if (c == ',') {
            read_punctuation(TokenType::COMMA);
        } else if (c == ':') {
            read_punctuation(TokenType::COLON);
        } else if (c == '!') {
            read_punctuation(TokenType::EXCLAMATION);
        } else if (c == '@') {
            read_punctuation(TokenType::AT);
        } else if (c == '(') {
            read_punctuation(TokenType::LPAREN);
        } else if (c == ')') {
            read_punctuation(TokenType::RPAREN);
        } else if (c == '{') {
            read_punctuation(TokenType::LBRACE);
        } else if (c == '}') {
            read_punctuation(TokenType::RBRACE);
        } else if (c == '[') {
            read_punctuation(TokenType::LBRACKET);
        } else if (c == ']') {
            read_punctuation(TokenType::RBRACKET);
        } else if (c == '\n') {
            read_end_of_line();
        } else if (c == '#') {
            skip_comment();
        } else {
            // TODO
            ASSERT_UNREACHABLE;
        }
    }

    for (unsigned i = 0; i < 2; i++) {
        tokens.push_back(Token{.type = TokenType::END_OF_FILE, .value{}});
    }

    return TokenStream{
        .tokens = std::move(tokens),
        .position = 0,
    };
}

void GenericLexer::read_identifier() {
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

void GenericLexer::read_number() {
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

void GenericLexer::read_punctuation(TokenType type) {
    consume();
    emit_token(type);
}

void GenericLexer::read_end_of_line() {
    consume();
    emit_token(TokenType::END_OF_LINE);
}

void GenericLexer::skip_whitespace() {
    while (position < source.size()) {
        char c = get();

        if (c != ' ' && c != '\r' && c != '\t') {
            break;
        }

        c = consume();
    }
}

void GenericLexer::skip_comment() {
    while (position < source.size()) {
        char c = consume();

        if (c == '\n') {
            break;
        }
    }
}

void GenericLexer::emit_token(TokenType type) {
    Token token{
        .type = type,
        .value = source.substr(token_start, position - token_start),
    };

    tokens.push_back(token);
}

} // namespace banjo::utils
