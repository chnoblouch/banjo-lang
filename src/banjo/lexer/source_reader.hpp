#ifndef BANJO_LEXER_SOURCE_READER_H
#define BANJO_LEXER_SOURCE_READER_H

#include "banjo/source/source_file.hpp"

namespace banjo {

namespace lang {

class SourceReader {

private:
    std::string_view buffer;
    unsigned position = 0;

public:
    SourceReader(const SourceFile &file) : buffer(file.get_buffer()) {}

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
