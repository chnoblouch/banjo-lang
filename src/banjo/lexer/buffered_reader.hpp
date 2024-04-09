#ifndef BUFFERED_READER_H
#define BUFFERED_READER_H

#include <istream>
#include <vector>

namespace lang {

class BufferedReader {

private:
    std::istream &stream;
    std::vector<char> buffer;
    unsigned position = 0;

public:
    BufferedReader(std::istream &stream);

    int consume() { return position >= buffer.size() ? EOF : buffer[position++]; }
    int get() { return position >= buffer.size() ? EOF : buffer[position]; }

    int peek() {
        unsigned peek_pos = position + 1;
        return peek_pos >= buffer.size() ? EOF : buffer[peek_pos];
    }

    int get_position() { return position; }
    void close() { position = buffer.size(); }

private:
    void read_all();
};

} // namespace lang

#endif
