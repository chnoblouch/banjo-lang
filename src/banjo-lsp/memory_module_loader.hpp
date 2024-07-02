#ifndef LSP_MEMORY_MODULE_LOADER_H
#define LSP_MEMORY_MODULE_LOADER_H

#include "source/module_loader.hpp"

namespace banjo {

namespace lsp {

class SourceManager;
struct SourceFile;

class MemoryModuleLoader : public lang::ModuleLoader {

private:
    SourceManager &source_manager;

public:
    MemoryModuleLoader(SourceManager &source_manager);
    std::istream *open(const lang::ModuleFile &module_file) override;
    void after_load(const lang::ModuleFile &module_file, lang::ASTModule *mod, bool is_valid) override;

private:
    SourceFile *find_lsp_source_file(const lang::ModuleFile &module_file, const lang::ModulePath &path);
};

} // namespace lsp

} // namespace banjo

#endif
