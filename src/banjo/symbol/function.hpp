#ifndef FUNCTION_H
#define FUNCTION_H

#include "ir/function.hpp"
#include "symbol/data_type.hpp"
#include "symbol/generics.hpp"
#include "symbol/symbol.hpp"
#include "symbol/symbol_ref.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lang {

class ASTModule;
class Structure;

struct Modifiers {
public:
    bool native = false;
    bool exposed = false;
    bool dllexport = false;
    bool method = false;
    bool test = false;
};

class Function : public Symbol {

private:
    ASTModule *module_;
    FunctionType type;
    std::vector<std::string_view> param_names;
    Modifiers modifiers;
    SymbolRef enclosing_symbol = {};
    GenericFuncInstance *generic_instance = nullptr;
    std::optional<std::string> link_name;

    bool used = false;
    bool return_by_ref = false;
    ir::Function *ir_func = nullptr;

public:
    Function() : Symbol(nullptr, "") {}
    Function(ASTNode *node, std::string name, ASTModule *module_) : Symbol(node, std::move(name)), module_(module_) {}

    ASTModule *get_module() { return module_; }
    FunctionType &get_type() { return type; }
    const std::vector<std::string_view> &get_param_names() { return param_names; }
    Modifiers &get_modifiers() { return modifiers; }
    bool is_native() { return modifiers.native; }
    bool is_exposed() { return modifiers.exposed; }
    bool is_method() { return modifiers.method; }
    bool is_dllexport() { return modifiers.dllexport; }
    const SymbolRef &get_enclosing_symbol() { return enclosing_symbol; }
    GenericFuncInstance *get_generic_instance() { return generic_instance; }
    std::optional<std::string> get_link_name() { return link_name; }
    bool is_used() { return used; }
    bool is_return_by_ref() { return return_by_ref; }
    ir::Function *get_ir_func() { return ir_func; }

    void set_param_names(std::vector<std::string_view> param_names) { this->param_names = std::move(param_names); }
    void set_modifiers(Modifiers modifiers) { this->modifiers = modifiers; }
    void set_enclosing_symbol(SymbolRef enclosing_symbol) { this->enclosing_symbol = enclosing_symbol; }
    void set_generic_instance(GenericFuncInstance *instance) { this->generic_instance = instance; }
    void set_link_name(std::optional<std::string> link_name) { this->link_name = std::move(link_name); }
    void set_used(bool used) { this->used = used; }
    void set_return_by_ref(bool return_by_ref) { this->return_by_ref = return_by_ref; }
    void set_ir_func(ir::Function *ir_func) { this->ir_func = ir_func; }
};

} // namespace lang

#endif
