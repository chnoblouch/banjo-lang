#include "memory_module_loader.hpp"

#include "banjo/ast/ast_module.hpp"
#include "workspace.hpp"

#include <sstream>

namespace banjo {

namespace lsp {

MemoryModuleLoader::MemoryModuleLoader(Workspace &workspace) : workspace(workspace) {}

std::istream *MemoryModuleLoader::open(const lang::ModuleFile &module_file) {
    File *file = workspace.find_or_load_file(module_file.file_path);
    if (!file) {
        return nullptr;
    }

    return new std::stringstream(file->content);
}

} // namespace lsp

} // namespace banjo
