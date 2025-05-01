#ifndef BANJO_LEXER_SOURCE_READER_H
#define BANJO_LEXER_SOURCE_READER_H

#include <istream>
#include <string>

namespace banjo {

namespace lang {

class SourceReader {

public:
    static constexpr char EOF_CHAR = '\0';

private:
    std::string buffer;
    unsigned position = 0;

public:
    static SourceReader read(std::istream &stream);

private:
    SourceReader(std::string buffer);

public:
    char consume() { return buffer[position++]; }
    char get() { return buffer[position]; }
    char peek() { return buffer[position + 1]; }

    unsigned get_position() { return position; }
    std::string_view value(unsigned start) { return std::string_view{&buffer[start], &buffer[position]}; }
};

} // namespace lang

} // namespace banjo

#endif
