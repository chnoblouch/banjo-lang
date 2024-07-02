#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/source/module_manager.hpp"

namespace banjo {

namespace lang {

class ModuleLoader {

public:
    virtual ~ModuleLoader() = default;
    virtual std::istream *open(const ModuleFile &module_file) = 0;
    virtual void after_load(const ModuleFile &module_file, ASTModule *mod, bool is_valid) {}
};

} // namespace lang

} // namespace banjo

#endif
