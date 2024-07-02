#include "buffered_reader.hpp"

#include "utils/timing.hpp"

namespace banjo {

namespace lang {

BufferedReader::BufferedReader(std::istream &stream) : stream(stream) {
    read_all();
}

void BufferedReader::read_all() {
    PROFILE_SCOPE("file loading");

    stream.seekg(0, std::ios::end);
    std::istream::pos_type size = stream.tellg();
    buffer.resize(size);
    stream.seekg(0, std::ios::beg);
    stream.read(buffer.data(), size);
}

} // namespace lang

} // namespace banjo
