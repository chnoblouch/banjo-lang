#ifndef BANJO_LEXER_H
#define BANJO_LEXER_H

#include "banjo/lexer/source_reader.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/source/source_file.hpp"

#include <vector>

namespace banjo {

namespace lang {

class Lexer {

public:
    enum class Mode {
        COMPILATION,
        FORMATTING,
    };

private:
    SourceReader reader;
    Mode mode;

    bool current_line_empty = true;
    bool previous_line_empty = false;

    bool completion_enabled = false;
    TextPosition completion_point;
    bool completion_token_inserted = false;

    std::vector<Token> tokens;

    TextPosition start_position;

public:
    Lexer(const SourceFile &file, Mode mode = Mode::COMPILATION);
    void enable_completion(TextPosition completion_point);
    std::vector<Token> tokenize();

private:
    void read_token();
    void read_newline();
    void read_whitespace();
    void read_identifier();
    void read_number();
    void read_character();
    void read_string();
    void skip_comment();
    void read_punctuation();

    void finish_token(TokenType type);
    void finish_line();
    void try_insert_completion_token();

    bool is_whitespace_char(char c);
    bool is_identifier_char(char c);
    bool is_number_char(char c);
    bool read_if(char c);
};

} // namespace lang

} // namespace banjo

#endif
