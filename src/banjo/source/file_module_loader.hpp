#ifndef FILE_MODULE_LOADER_H
#define FILE_MODULE_LOADER_H

#include "banjo/source/module_loader.hpp"

namespace banjo {

namespace lang {

class FileModuleLoader : public ModuleLoader {

public:
    std::istream *open(const ModuleFile &module_file) override;
};

} // namespace lang

} // namespace banjo

#endif
