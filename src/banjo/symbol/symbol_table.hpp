#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "banjo/symbol/constant.hpp"
#include "banjo/symbol/function.hpp"
#include "banjo/symbol/generics.hpp"
#include "banjo/symbol/global_variable.hpp"
#include "banjo/symbol/local_variable.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/symbol/parameter.hpp"
#include "banjo/symbol/structure.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/symbol/type_alias.hpp"
#include "banjo/symbol/union.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace lang {

typedef std::unordered_map<std::string, SymbolRef> SymbolMap;

class Enumeration;

class SymbolTable {

private:
    SymbolTable *parent;
    std::vector<Function *> functions;
    std::vector<LocalVariable *> local_variables;
    std::vector<Parameter *> parameters;
    std::vector<Structure *> structures;
    std::vector<GenericFunc *> generic_funcs;
    std::vector<GenericStruct *> generic_structs;

    SymbolMap symbols;

public:
    SymbolTable(SymbolTable *parent);
    ~SymbolTable();

    SymbolTable *get_parent() { return parent; }
    void set_parent(SymbolTable *parent) { this->parent = parent; }

    std::vector<Function *> &get_functions() { return functions; }
    void add_function(Function *function);
    Function *get_function(const std::string &name);
    std::vector<Function *> get_functions(const std::string &name);
    Function *get_function_with_params(const std::string &name, const std::vector<DataType *> &parameters);

    std::vector<LocalVariable *> &get_local_variables() { return local_variables; }
    LocalVariable *get_local_variable(const std::string &name);
    void add_local_variable(LocalVariable *local_variable);

    std::vector<Parameter *> &get_parameters() { return parameters; }
    Parameter *get_parameter(const std::string &name);
    void add_parameter(Parameter *parameter);

    void add_global_variable(GlobalVariable *global_variable);
    void add_constant(Constant *constant);

    std::vector<Structure *> &get_structures() { return structures; }
    Structure *get_structure(const std::string &name);
    void add_structure(Structure *structure);

    void add_enumeration(Enumeration *enumeration);
    void add_enum_variant(EnumVariant *enum_variant);
    void add_union(Union *union_);
    void add_protocol(Protocol *protocol);
    void add_type_alias(TypeAlias *type_alias);

    std::vector<GenericFunc *> &get_generic_funcs() { return generic_funcs; }
    GenericFunc *get_generic_func(const std::string &name);
    void add_generic_func(GenericFunc *generic_func);

    std::vector<GenericStruct *> &get_generic_structs() { return generic_structs; }
    GenericStruct *get_generic_struct(const std::string &name);
    void add_generic_struct(GenericStruct *generic_struct);

    const SymbolMap &get_symbols() { return symbols; }
    std::optional<SymbolRef> get_symbol(const std::string &name);
    std::optional<SymbolRef> get_symbol_unresolved(const std::string &name);
    void add_symbol(std::string name, SymbolRef symbol);
    void add_groupable_symbol(std::string name, SymbolRef symbol);
    void replace_symbol(const std::string &name, SymbolRef symbol);

private:
    static bool does_param_list_match(const std::vector<DataType *> &a, const std::vector<DataType *> &b);

    template <typename SymbolType, std::vector<SymbolType *> SymbolTable::*ListMember>
    SymbolType *get_symbol(const std::string &name) {
        std::vector<SymbolType *> symbols = this->*ListMember;

        for (SymbolType *symbol : symbols) {
            if (symbol->get_name() == name) {
                return symbol;
            }
        }

        if (parent) {
            return parent->get_symbol<SymbolType, ListMember>(name);
        }

        return nullptr;
    }
};

} // namespace lang

} // namespace banjo

#endif
