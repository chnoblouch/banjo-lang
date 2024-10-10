#ifndef LSP_MEMORY_MODULE_LOADER_H
#define LSP_MEMORY_MODULE_LOADER_H

#include "banjo/source/module_loader.hpp"

namespace banjo {

namespace lsp {

class Workspace;

class MemoryModuleLoader : public lang::ModuleLoader {

private:
    Workspace &workspace;

public:
    MemoryModuleLoader(Workspace &workspace);
    std::istream *open(const lang::ModuleFile &module_file) override;
};

} // namespace lsp

} // namespace banjo

#endif
