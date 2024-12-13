#include "lexer.hpp"

#include "banjo/config/config.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/timing.hpp"

#include <iostream>
#include <map>
#include <string_view>

namespace banjo {

namespace lang {

const std::map<std::string_view, TokenType> KEYWORDS{
    {"var", TKN_VAR},       {"const", TKN_CONST},   {"func", TKN_FUNC},     {"i8", TKN_I8},
    {"i16", TKN_I16},       {"i32", TKN_I32},       {"i64", TKN_I64},       {"u8", TKN_U8},
    {"u16", TKN_U16},       {"u32", TKN_U32},       {"u64", TKN_U64},       {"f32", TKN_F32},
    {"f64", TKN_F64},       {"usize", TKN_USIZE},   {"bool", TKN_BOOL},     {"addr", TKN_ADDR},
    {"void", TKN_VOID},     {"except", TKN_EXCEPT}, {"as", TKN_AS},         {"if", TKN_IF},
    {"else", TKN_ELSE},     {"switch", TKN_SWITCH}, {"try", TKN_TRY},       {"while", TKN_WHILE},
    {"for", TKN_FOR},       {"in", TKN_IN},         {"break", TKN_BREAK},   {"continue", TKN_CONTINUE},
    {"return", TKN_RETURN}, {"struct", TKN_STRUCT}, {"self", TKN_SELF},     {"enum", TKN_ENUM},
    {"union", TKN_UNION},   {"case", TKN_CASE},     {"proto", TKN_PROTO},   {"false", TKN_FALSE},
    {"true", TKN_TRUE},     {"null", TKN_NULL},     {"none", TKN_NONE},     {"undefined", TKN_UNDEFINED},
    {"use", TKN_USE},       {"pub", TKN_PUB},       {"native", TKN_NATIVE}, {"meta", TKN_META},
    {"type", TKN_TYPE},
};

Lexer::Lexer(std::istream &input_stream) : reader(input_stream) {}

void Lexer::enable_completion(TextPosition completion_point) {
    completion_enabled = true;
    this->completion_point = completion_point;
}

std::vector<Token> Lexer::tokenize() {
    PROFILE_SCOPE("lexer");

    tokens.clear();
    start_position = reader.get_position();

    while (reader.get() != EOF) {
        read_token();
        start_position = reader.get_position();
        try_insert_completion_token();
    }

    finish_line();
    return tokens;
}

void Lexer::read_token() {
    char c = reader.get();

    if (c == '\n') read_newline();
    else if (is_whitespace_char(c)) read_whitespace();
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') read_identifier();
    else if (c >= '0' && c <= '9') read_number();
    else if (c == '\'') read_character();
    else if (c == '\"') read_string();
    else if (c == '#') skip_comment();
    else read_punctuation();
}

void Lexer::read_newline() {
    reader.consume();
    finish_line();
}

void Lexer::read_whitespace() {
    while (is_whitespace_char(reader.get())) {
        reader.consume();
    }
}

void Lexer::read_identifier() {
    while (is_identifier_char(reader.get())) {
        token_builder += reader.consume();
    }

    auto iter = KEYWORDS.find(token_builder);
    finish_token(iter == KEYWORDS.end() ? TKN_IDENTIFIER : iter->second);
}

void Lexer::read_number() {
    while (is_number_char(reader.get())) {
        if (reader.get() == '.' && reader.peek() == '.') {
            break;
        }

        token_builder += reader.consume();
    }

    finish_token(TKN_LITERAL);
}

void Lexer::read_character() {
    token_builder += reader.consume();

    char c = (char)reader.consume();
    while (c != EOF) {
        token_builder += c;
        if (c == '\'') {
            break;
        }

        c = (char)reader.consume();
    }

    finish_token(TKN_CHARACTER);
}

void Lexer::read_string() {
    token_builder += reader.consume();

    char c = (char)reader.consume();
    while (c != EOF) {
        if (c == '\\') {
            c = (char)reader.consume();
            if (c == EOF) break;
            else if (c == '\"') token_builder += '\"';
            else token_builder += std::string("\\") + c;
        } else {
            token_builder += c;
            if (c == '\"') {
                break;
            }
        }

        c = (char)reader.consume();
    }

    finish_token(TKN_STRING);
}

void Lexer::skip_comment() {
    char c = (char)reader.consume();
    while (c != '\n' && c != EOF) {
        c = (char)reader.consume();
    }
}

void Lexer::read_punctuation() {
    char c = reader.consume();
    token_builder += c;

    TokenType type;

    switch (c) {
        case '+': type = read_if('=') ? TKN_PLUS_EQ : TKN_PLUS; break;
        case '-':
            if (read_if('>')) type = TKN_ARROW;
            else if (read_if('=')) type = TKN_MINUS_EQ;
            else type = TKN_MINUS;
            break;
        case '*': type = read_if('=') ? TKN_STAR_EQ : TKN_STAR; break;
        case '/': type = read_if('=') ? TKN_SLASH_EQ : TKN_SLASH; break;
        case '%': type = read_if('=') ? TKN_PERCENT_EQ : TKN_PERCENT; break;
        case '&':
            if (read_if('&')) type = TKN_AND_AND;
            else if (read_if('=')) type = TKN_AND_EQ;
            else type = TKN_AND;
            break;
        case '|':
            if (read_if('|')) type = TKN_OR_OR;
            else if (read_if('=')) type = TKN_OR_EQ;
            else type = TKN_OR;
            break;
        case '!': type = read_if('=') ? TKN_NE : TKN_NOT; break;
        case '^': type = read_if('=') ? TKN_CARET_EQ : TKN_CARET; break;
        case '~': type = TKN_TILDE; break;
        case '>':
            if (read_if('>')) type = read_if('=') ? TKN_SHR_EQ : TKN_SHR;
            else if (read_if('=')) type = TKN_GE;
            else type = TKN_GT;
            break;
        case '<':
            if (read_if('<')) type = read_if('=') ? TKN_SHL_EQ : TKN_SHL;
            else if (read_if('=')) type = TKN_LE;
            else type = TKN_LT;
            break;
        case '=': type = read_if('=') ? TKN_EQ_EQ : TKN_EQ; break;
        case '(': type = TKN_LPAREN; break;
        case ')': type = TKN_RPAREN; break;
        case '{': type = TKN_LBRACE; break;
        case '}': type = TKN_RBRACE; break;
        case '[': type = TKN_LBRACKET; break;
        case ']': type = TKN_RBRACKET; break;
        case ';': type = TKN_SEMI; break;
        case ':': type = TKN_COLON; break;
        case ',': type = TKN_COMMA; break;
        case '.':
            if (read_if('.')) type = read_if('.') ? TKN_DOT_DOT_DOT : TKN_DOT_DOT;
            else type = TKN_DOT;
            break;
        case '?': type = TKN_QUESTION; break;
        case '@': type = TKN_AT; break;
        default: return;
    }

    finish_token(type);
}

void Lexer::finish_token(TokenType type) {
    tokens.push_back({type, token_builder, start_position});
    token_builder.clear();
}

void Lexer::finish_line() {
    if (Config::instance().optional_semicolons && !tokens.empty()) {
        tokens.back().end_of_line = true;
    }
}

void Lexer::try_insert_completion_token() {
    if (completion_enabled && !completion_token_inserted && reader.get_position() >= completion_point) {
        // Delete the previous token if it's an identifier and if the completion point is at its end
        if (!tokens.empty()) {
            Token &back = tokens.back();
            if (back.get_type() == TKN_IDENTIFIER && completion_point == tokens.back().get_range().end) {
                tokens.pop_back();
            }
        }

        tokens.emplace_back(TKN_COMPLETION, "", reader.get_position());
        completion_token_inserted = true;
    }
}

bool Lexer::is_whitespace_char(char c) {
    return c == ' ' || c == '\r' || c == '\t';
}

bool Lexer::is_identifier_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool Lexer::is_number_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'x' || c == 'b' ||
           c == '.';
}

bool Lexer::read_if(char c) {
    if (reader.get() == c) {
        reader.consume();
        token_builder.push_back(c);
        return true;
    }

    return false;
}

} // namespace lang

} // namespace banjo
