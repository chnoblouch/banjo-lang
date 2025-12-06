#include "lexer.hpp"

#include "banjo/config/config.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/timing.hpp"

#include <map>
#include <string_view>

namespace banjo {

namespace lang {

const std::map<std::string_view, TokenType> KEYWORDS{
    {"var", TKN_VAR},       {"const", TKN_CONST},   {"func", TKN_FUNC},
    {"i8", TKN_I8},         {"i16", TKN_I16},       {"i32", TKN_I32},
    {"i64", TKN_I64},       {"u8", TKN_U8},         {"u16", TKN_U16},
    {"u32", TKN_U32},       {"u64", TKN_U64},       {"f32", TKN_F32},
    {"f64", TKN_F64},       {"usize", TKN_USIZE},   {"bool", TKN_BOOL},
    {"addr", TKN_ADDR},     {"void", TKN_VOID},     {"except", TKN_EXCEPT},
    {"ref", TKN_REF},       {"mut", TKN_MUT},       {"as", TKN_AS},
    {"if", TKN_IF},         {"else", TKN_ELSE},     {"switch", TKN_SWITCH},
    {"try", TKN_TRY},       {"while", TKN_WHILE},   {"for", TKN_FOR},
    {"in", TKN_IN},         {"break", TKN_BREAK},   {"continue", TKN_CONTINUE},
    {"return", TKN_RETURN}, {"struct", TKN_STRUCT}, {"self", TKN_SELF},
    {"enum", TKN_ENUM},     {"union", TKN_UNION},   {"case", TKN_CASE},
    {"proto", TKN_PROTO},   {"false", TKN_FALSE},   {"true", TKN_TRUE},
    {"null", TKN_NULL},     {"none", TKN_NONE},     {"undefined", TKN_UNDEFINED},
    {"use", TKN_USE},       {"pub", TKN_PUB},       {"native", TKN_NATIVE},
    {"meta", TKN_META},     {"type", TKN_TYPE},
};

Lexer::Lexer(const SourceFile &file, Mode mode /*= Mode::COMPILATION*/) : reader{file}, mode(mode) {}

void Lexer::enable_completion(TextPosition completion_point) {
    completion_enabled = true;
    this->completion_point = completion_point;
}

TokenList Lexer::tokenize() {
    PROFILE_SCOPE("lexer");

    tokens.clear();
    start_position = reader.get_position();

    if (mode == Mode::KEEP_WHITESPACE) {
        attachments.push_back(TokenList::Span{.first = 0, .count = 0});
    }

    while (reader.get() != SourceFile::EOF_CHAR) {
        read_token();
        start_position = reader.get_position();
        try_insert_completion_token();
    }

    finish_line();

    for (unsigned i = 0; i < TokenList::EOF_ZONE_SIZE; i++) {
        tokens.push_back(Token{TKN_EOF, {}, reader.get_position()});
    }

    return TokenList{
        .tokens = std::move(tokens),
        .attached_tokens = std::move(attached_tokens),
        .attachments = std::move(attachments),
    };
}

void Lexer::read_token() {
    char c = reader.get();

    if (is_whitespace_char(c)) read_whitespace();
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') read_identifier();
    else if (c >= '0' && c <= '9') read_number();
    else if (c == '\'') read_character();
    else if (c == '\"') read_string();
    else if (c == '#') read_comment();
    else read_punctuation();
}

void Lexer::read_whitespace() {
    char c = reader.get();
    bool end_of_line = false;

    while (is_whitespace_char(c)) {
        if (c == '\n') {
            end_of_line = true;
        }

        reader.consume();
        c = reader.get();
    }

    if (end_of_line) {
        finish_line();
    }

    attach_token(TKN_WHITESPACE);
}

void Lexer::read_identifier() {
    while (is_identifier_char(reader.get())) {
        reader.consume();
    }

    std::string_view value = reader.value(start_position);
    auto iter = KEYWORDS.find(value);
    finish_token(iter == KEYWORDS.end() ? TKN_IDENTIFIER : iter->second);
}

void Lexer::read_number() {
    while (is_number_char(reader.get())) {
        if (reader.get() == '.' && reader.peek() == '.') {
            break;
        }

        reader.consume();
    }

    finish_token(TKN_LITERAL);
}

void Lexer::read_character() {
    reader.consume();

    char c = reader.consume();
    while (c != SourceFile::EOF_CHAR) {
        if (c == '\'') {
            break;
        }

        c = reader.consume();
    }

    finish_token(TKN_CHARACTER);
}

void Lexer::read_string() {
    reader.consume();

    char c = reader.consume();
    while (c != SourceFile::EOF_CHAR) {
        if (c == '\\') {
            c = reader.consume();
            if (c == SourceFile::EOF_CHAR) break;
        } else if (c == '\"') {
            break;
        }

        c = reader.consume();
    }

    finish_token(TKN_STRING);
}

void Lexer::read_comment() {
    while (reader.get() != '\n' && reader.get() != SourceFile::EOF_CHAR) {
        reader.consume();
    }

    attach_token(TKN_COMMENT);
}

void Lexer::read_punctuation() {
    TokenType type;

    switch (reader.consume()) {
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
    tokens.push_back(Token{type, reader.value(start_position), start_position});

    if (mode == Mode::KEEP_WHITESPACE) {
        attachments.push_back(TokenList::Span{.first = 0, .count = 0});
    }
}

void Lexer::finish_line() {
    if (Config::instance().optional_semicolons && !tokens.empty()) {
        tokens.back().end_of_line = true;
    }
}

void Lexer::attach_token(TokenType type) {
    if (mode != Mode::KEEP_WHITESPACE) {
        return;
    }

    attached_tokens.push_back(Token{type, reader.value(start_position), start_position});

    TokenList::Span &attachment = attachments.back();

    if (attachment.count == 0) {
        attachment.first = static_cast<unsigned>(attached_tokens.size() - 1);
        attachment.count = 1;
    } else {
        attachment.count += 1;
    }
}

void Lexer::try_insert_completion_token() {
    if (completion_enabled && !completion_token_inserted && reader.get_position() >= completion_point) {
        // Delete the previous token if it's an identifier and if the completion point is at its end
        if (!tokens.empty()) {
            Token &back = tokens.back();
            if (back.is(TKN_IDENTIFIER) && completion_point == tokens.back().range().end) {
                tokens.pop_back();
            }
        }

        tokens.push_back(Token{TKN_COMPLETION, "", reader.get_position()});
        completion_token_inserted = true;
    }
}

bool Lexer::is_whitespace_char(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool Lexer::is_identifier_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool Lexer::is_number_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'x' || c == 'b' ||
           c == 'o' || c == '.';
}

bool Lexer::read_if(char c) {
    if (reader.get() == c) {
        reader.consume();
        return true;
    }

    return false;
}

} // namespace lang

} // namespace banjo
