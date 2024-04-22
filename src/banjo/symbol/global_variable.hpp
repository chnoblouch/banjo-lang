#ifndef GLOBAL_VARIABLE_H
#define GLOBAL_VARIABLE_H

#include "symbol/variable.hpp"

#include <optional>
#include <string>
#include <utility>

namespace lang {

class ASTModule;

class GlobalVariable : public Variable {

private:
    ASTModule *module_;
    bool native = false;
    bool exposed = false;
    std::optional<std::string> link_name;

public:
    GlobalVariable(ASTNode *node, std::string name, ASTModule *module_);
    ASTModule *get_module() { return module_; }
    bool is_native() { return native; }
    bool is_exposed() { return exposed; }
    std::optional<std::string> get_link_name() { return link_name; }

    void set_native(bool native) { this->native = native; }
    void set_exposed(bool exposed) { this->exposed = exposed; }
    void set_link_name(std::optional<std::string> link_name) { this->link_name = std::move(link_name); }

    ir_builder::StoredValue as_ir_value(ir_builder::IRBuilderContext &context) override final;
};

} // namespace lang

#endif
