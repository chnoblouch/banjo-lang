#ifndef BANJO_LEXER_TOKEN_H
#define BANJO_LEXER_TOKEN_H

#include "banjo/source/text_range.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

enum TokenType : std::uint8_t {
    TKN_IDENTIFIER,
    TKN_LITERAL,
    TKN_CHARACTER,
    TKN_STRING,
    TKN_COMPLETION,
    TKN_EOF,

    TKN_VAR,
    TKN_CONST,
    TKN_FUNC,
    TKN_I8,
    TKN_I16,
    TKN_I32,
    TKN_I64,
    TKN_U8,
    TKN_U16,
    TKN_U32,
    TKN_U64,
    TKN_F32,
    TKN_F64,
    TKN_USIZE,
    TKN_BOOL,
    TKN_ADDR,
    TKN_VOID,
    TKN_EXCEPT,
    TKN_REF,
    TKN_MUT,
    TKN_AS,
    TKN_IF,
    TKN_ELSE,
    TKN_SWITCH,
    TKN_TRY,
    TKN_WHILE,
    TKN_FOR,
    TKN_IN,
    TKN_BREAK,
    TKN_CONTINUE,
    TKN_RETURN,
    TKN_STRUCT,
    TKN_SELF,
    TKN_ENUM,
    TKN_UNION,
    TKN_CASE,
    TKN_PROTO,
    TKN_FALSE,
    TKN_TRUE,
    TKN_NULL,
    TKN_NONE,
    TKN_UNDEFINED,
    TKN_USE,
    TKN_PUB,
    TKN_NATIVE,
    TKN_META,
    TKN_TYPE,

    TKN_PLUS,
    TKN_MINUS,
    TKN_STAR,
    TKN_SLASH,
    TKN_PERCENT,
    TKN_AND,
    TKN_OR,
    TKN_NOT,
    TKN_SHL,
    TKN_SHR,
    TKN_CARET,
    TKN_TILDE,
    TKN_EQ,
    TKN_EQ_EQ,
    TKN_NE,
    TKN_GT,
    TKN_LT,
    TKN_GE,
    TKN_LE,
    TKN_AND_AND,
    TKN_OR_OR,
    TKN_DOT,
    TKN_DOT_DOT,
    TKN_DOT_DOT_DOT,
    TKN_PLUS_EQ,
    TKN_MINUS_EQ,
    TKN_STAR_EQ,
    TKN_SLASH_EQ,
    TKN_PERCENT_EQ,
    TKN_AND_EQ,
    TKN_OR_EQ,
    TKN_CARET_EQ,
    TKN_SHL_EQ,
    TKN_SHR_EQ,
    TKN_SEMI,
    TKN_LPAREN,
    TKN_RPAREN,
    TKN_LBRACE,
    TKN_RBRACE,
    TKN_LBRACKET,
    TKN_RBRACKET,
    TKN_COLON,
    TKN_ARROW,
    TKN_COMMA,
    TKN_QUESTION,
    TKN_AT,

    TKN_WHITESPACE,
    TKN_COMMENT,
};

struct Token {
    TokenType type;
    std::string_view value;
    TextPosition position;
    bool end_of_line = false;
    bool after_empty_line = false;

    Token(TokenType type, std::string_view value, TextPosition position)
      : type(type),
        value(value),
        position(position) {}

    TextPosition end() const { return position + value.size(); }
    TextRange range() const { return TextRange{position, end()}; }

    bool is(TokenType type) const { return this->type == type; }
};

struct TokenList {
    struct Span {
        unsigned first;
        unsigned count;
    };

    std::vector<Token> tokens;
    std::vector<Token> attached_tokens;
    std::vector<Span> attachments;

    std::span<Token> get_attached_tokens(unsigned token_index) {
        TokenList::Span span = attachments[token_index];
        return std::span<Token>{&attached_tokens[span.first], span.count};
    }
};

} // namespace lang

} // namespace banjo

#endif
