#ifndef FILE_MODULE_LOADER_H
#define FILE_MODULE_LOADER_H

#include "source/module_loader.hpp"

namespace lang {

class FileModuleLoader : public ModuleLoader {

public:
    std::istream *open(const ModuleFile &module_file) override;
};

} // namespace lang

#endif
