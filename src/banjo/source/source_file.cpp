#include "source_file.hpp"

#include "banjo/utils/timing.hpp"

#include <utility>

namespace banjo::lang {

SourceFile SourceFile::read(ModulePath mod_path, std::filesystem::path fs_path, std::istream &stream) {
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

    return SourceFile{
        .mod_path = std::move(mod_path),
        .fs_path = std::move(fs_path),
        .buffer = buffer,
        .tokens{},
        .ast_mod = nullptr,
        .sir_mod = nullptr,
    };
}

} // namespace banjo::lang
