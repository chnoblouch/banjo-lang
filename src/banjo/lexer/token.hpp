#ifndef TOKEN_H
#define TOKEN_H

#include "banjo/source/text_range.hpp"
#include <string>

namespace banjo {

namespace lang {

enum TokenType {
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
    TKN_AT
};

class Token {

public:
    TokenType type;
    std::string value;
    TextPosition position;
    bool end_of_line = false;

public:
    Token(TokenType type, std::string value, TextPosition position)
      : type(type),
        value(std::move(value)),
        position(position) {}

public:
    TokenType get_type() { return type; }
    const std::string &get_value() { return value; }
    std::string move_value() { return std::move(value); }
    TextPosition get_position() { return position; }
    TextPosition get_end() { return position + value.size(); }
    TextRange get_range() { return TextRange(position, get_end()); }

    bool is(TokenType type) { return this->type == type; }
};

} // namespace lang

} // namespace banjo

#endif
