#ifndef BANJO_AST_MODULE_LIST_H
#define BANJO_AST_MODULE_LIST_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/utils/timing.hpp"
#include "banjo/source/source_file.hpp"

#include <list>

namespace banjo {

namespace lang {

class ModuleList {

private:
    std::list<ASTModule *> modules;

public:
    ModuleList() {}
    ModuleList(const ModuleList &) = delete;
    ModuleList(ModuleList &&other) noexcept : modules(std::move(other.modules)) {}
    ~ModuleList() { clear(); }

    ModuleList &operator=(const ModuleList &) = delete;

    ModuleList &operator=(ModuleList &&other) noexcept {
        modules = std::move(other.modules);
        return *this;
    }

    void add(ASTModule *module_) { modules.push_back(module_); }

    void replace(ASTModule *old_module, ASTModule *new_module) {
        auto iter = std::find(modules.begin(), modules.end(), old_module);
        *iter = new_module;
    }

    void remove(ASTModule *module_) {
        modules.remove(module_);
        delete module_;
    }

    void clear() {
        PROFILE_SCOPE("module deletion");

        for (ASTModule *module_ : modules) {
            delete module_;
        }

        modules.clear();
    }

    ASTModule *get_by_path(const ModulePath &path) const {
        for (ASTModule *module_ : modules) {
            if (module_->file.mod_path == path) {
                return module_;
            }
        }

        return nullptr;
    }

    unsigned get_size() const { return modules.size(); }

    std::list<ASTModule *>::iterator begin() { return modules.begin(); }
    std::list<ASTModule *>::iterator end() { return modules.end(); }
    std::list<ASTModule *>::const_iterator begin() const { return modules.begin(); }
    std::list<ASTModule *>::const_iterator end() const { return modules.end(); }
};

} // namespace lang

} // namespace banjo

#endif
