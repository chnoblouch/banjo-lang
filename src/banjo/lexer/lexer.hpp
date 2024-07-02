#ifndef LEXER_H
#define LEXER_H

#include "lexer/buffered_reader.hpp"
#include "lexer/token.hpp"

#include <istream>
#include <vector>

namespace banjo {

namespace lang {

class Lexer {

private:
    BufferedReader reader;

    bool completion_enabled = false;
    TextPosition completion_point;
    bool completion_token_inserted = false;

    std::vector<Token> tokens;

    std::string token_builder;
    TextPosition start_position;

public:
    Lexer(std::istream &input_stream);
    void enable_completion(TextPosition completion_point);
    std::vector<Token> tokenize();

private:
    void read_token();
    void read_whitespace();
    void read_identifier();
    void read_number();
    void read_character();
    void read_string();
    void skip_comment();
    void read_punctuation();

    void finish_token(TokenType type);
    void try_insert_completion_token();

    bool is_whitespace_char(char c);
    bool is_identifier_char(char c);
    bool is_number_char(char c);
    bool read_if(char c);
};

} // namespace lang

} // namespace banjo

#endif
