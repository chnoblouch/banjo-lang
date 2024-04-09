#include "memory_module_loader.hpp"

#include "ast/ast_module.hpp"
#include "source_manager.hpp"
#include <sstream>

namespace lsp {

MemoryModuleLoader::MemoryModuleLoader(SourceManager &source_manager) : source_manager(source_manager) {}

std::istream *MemoryModuleLoader::open(const lang::ModuleFile &module_file) {
    SourceFile *source_file = find_lsp_source_file(module_file, module_file.path);
    if (!source_file) {
        return nullptr;
    }

    return new std::stringstream(source_file->source);
}

void MemoryModuleLoader::after_load(const lang::ModuleFile &module_file, lang::ASTModule *mod, bool is_valid) {
    SourceFile *source_file = find_lsp_source_file(module_file, mod->get_path());
    if (!source_file) {
        return;
    }

    source_file->module_node = mod;
    source_file->is_valid = is_valid;
}

SourceFile *MemoryModuleLoader::find_lsp_source_file(
    const lang::ModuleFile &module_file,
    const lang::ModulePath &path
) {
    std::filesystem::path absolute_path = std::filesystem::absolute(module_file.file_path);
    return &source_manager.get_or_load_file(path, absolute_path);
}

} // namespace lsp
