#include "file_module_loader.hpp"

#include <fstream>

namespace banjo {

namespace lang {

std::istream *FileModuleLoader::open(const ModuleFile &module_file) {
    return new std::ifstream(module_file.file_path, std::ios::binary);
}

} // namespace lang

} // namespace banjo
