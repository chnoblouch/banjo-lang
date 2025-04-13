#ifndef BANJO_TEST_UTIL_LINE_BASED_READER_H
#define BANJO_TEST_UTIL_LINE_BASED_READER_H

#include <istream>
#include <sstream>
#include <string>
#include <string_view>

namespace banjo {
namespace test {

class LineBasedReader {

private:
    std::stringstream stream;
    std::string line;
    unsigned char_index;

public:
    LineBasedReader(std::istream &source_stream);

    bool next_line();
    void restart_line();
    void reset();
    void skip_whitespace();
    void skip_char(char c);
    std::string_view read_until_whitespace();

    char get() { return line[char_index]; }
    char consume() { return line[char_index++]; }

    static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    static bool is_numeric(char c) { return c >= '0' && c <= '9'; }
    static bool is_alphanumeric(char c) { return is_alpha(c) || is_numeric(c); }
    static bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\0'; }
};

} // namespace test
} // namespace banjo

#endif
