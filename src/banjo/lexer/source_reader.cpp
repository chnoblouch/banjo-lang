#include "source_reader.hpp"

#include "banjo/utils/timing.hpp"

namespace banjo {

namespace lang {

static constexpr unsigned EOF_ZONE_SIZE = 2;

SourceReader SourceReader::read(std::istream &stream) {
    PROFILE_SCOPE("file loading");

    stream.seekg(0, std::ios::end);
    std::size_t file_size = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);

    std::string buffer;
    std::size_t buffer_size = file_size + EOF_ZONE_SIZE;
    buffer.resize(buffer_size);

    stream.read(buffer.data(), file_size);

    for (unsigned i = 0; i < EOF_ZONE_SIZE; i++) {
        buffer[file_size + i] = EOF_CHAR;
    }

    return SourceReader{std::move(buffer)};
}

SourceReader::SourceReader(std::string buffer) : buffer(std::move(buffer)) {}

} // namespace lang

} // namespace banjo
