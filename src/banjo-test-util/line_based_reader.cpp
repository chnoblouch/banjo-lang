#include "line_based_reader.hpp"

#include "banjo/utils/macros.hpp"

namespace banjo {
namespace test {

LineBasedReader::LineBasedReader(std::istream &source_stream) {
    std::string line;

    while (std::getline(source_stream, line)) {
        stream << line << '\n';
    }
}

bool LineBasedReader::next_line() {
    char_index = 0;
    return static_cast<bool>(std::getline(stream, line));
}

void LineBasedReader::restart_line() {
    char_index = 0;
}

void LineBasedReader::reset() {
    stream.clear();
    stream.seekg(0);
}

void LineBasedReader::skip_whitespace() {
    if (get() == '\0') {
        return;
    }

    while (get() == ' ' || get() == '\t') {
        char_index += 1;

        if (get() == '\0') {
            return;
        }
    }
}

void LineBasedReader::skip_char(char c) {
    ASSERT(consume() == c);
}

std::string_view LineBasedReader::read_until_whitespace() {
    unsigned start = char_index;

    while (!is_whitespace(get()) && get() != '\0') {
        consume();
    }

    return std::string_view(&line[start], &line[char_index]);
}

} // namespace test
} // namespace banjo
