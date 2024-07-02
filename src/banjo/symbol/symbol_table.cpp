#include "symbol_table.hpp"

#include "symbol/constant.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/function.hpp"
#include "symbol/global_variable.hpp"
#include "symbol/local_variable.hpp"
#include "symbol/parameter.hpp"
#include "symbol/structure.hpp"
#include "symbol/union.hpp"
#include "symbol/protocol.hpp"

#include <cassert>

namespace banjo {

namespace lang {

SymbolTable::SymbolTable(SymbolTable *parent) : parent(parent) {}

SymbolTable::~SymbolTable() {
    for (auto &pair : symbols) {
        pair.second.destroy();
    }
}

void SymbolTable::add_function(Function *function) {
    functions.push_back(function);
    add_groupable_symbol(function->get_name(), SymbolRef(function));
}

Function *SymbolTable::get_function(const std::string &name) {
    for (Function *function : functions) {
        if (function->get_name() == name) {
            return function;
        }
    }

    if (parent) {
        return parent->get_function(name);
    }

    return nullptr;
}

std::vector<Function *> SymbolTable::get_functions(const std::string &name) {
    std::vector<Function *> functions;
    for (Function *function : this->functions) {
        if (function->get_name() == name) {
            functions.push_back(function);
        }
    }

    if (parent) {
        std::vector<Function *> parent_functions = parent->get_functions(name);
        functions.insert(functions.end(), parent_functions.begin(), parent_functions.end());
    }

    return functions;
}

Function *SymbolTable::get_function_with_params(const std::string &name, const std::vector<DataType *> &parameters) {
    for (Function *function : functions) {
        if (function->get_name() == name) {
            return DataType::equal(function->get_type().param_types, parameters) ? function : nullptr;
        }
    }

    if (parent) {
        return parent->get_function_with_params(name, parameters);
    }

    return nullptr;
}

LocalVariable *SymbolTable::get_local_variable(const std::string &name) {
    return get_symbol<LocalVariable, &SymbolTable::local_variables>(name);
}

void SymbolTable::add_local_variable(LocalVariable *local_variable) {
    local_variables.push_back(local_variable);
    symbols.insert({local_variable->get_name(), local_variable});
}

Parameter *SymbolTable::get_parameter(const std::string &name) {
    return get_symbol<Parameter, &SymbolTable::parameters>(name);
}

void SymbolTable::add_parameter(Parameter *parameter) {
    parameters.push_back(parameter);
    symbols.insert({parameter->get_name(), parameter});
}

void SymbolTable::add_global_variable(GlobalVariable *global_variable) {
    symbols.insert({global_variable->get_name(), global_variable});
}

void SymbolTable::add_constant(Constant *constant) {
    symbols.insert({constant->get_name(), constant});
}

Structure *SymbolTable::get_structure(const std::string &name) {
    for (Structure *structure : structures) {
        if (structure->get_name() == name) {
            return structure;
        }
    }

    if (parent) {
        return parent->get_structure(name);
    }

    return nullptr;
}

void SymbolTable::add_structure(Structure *structure) {
    structures.push_back(structure);
    symbols.insert({structure->get_name(), structure});
}

void SymbolTable::add_enumeration(Enumeration *enumeration) {
    symbols.insert({enumeration->get_name(), enumeration});
}

void SymbolTable::add_enum_variant(EnumVariant *enum_variant) {
    symbols.insert({enum_variant->get_name(), enum_variant});
}

void SymbolTable::add_union(Union *union_) {
    symbols.insert({union_->get_name(), union_});
}

void SymbolTable::add_protocol(Protocol *protocol) {
    symbols.insert({protocol->get_name(), protocol});
}

void SymbolTable::add_type_alias(TypeAlias *type_alias) {
    symbols.insert({type_alias->get_name(), type_alias});
}

GenericFunc *SymbolTable::get_generic_func(const std::string &name) {
    return get_symbol<GenericFunc, &SymbolTable::generic_funcs>(name);
}

void SymbolTable::add_generic_func(GenericFunc *generic_func) {
    generic_funcs.push_back(generic_func);
    symbols.insert({generic_func->get_name(), generic_func});
}

GenericStruct *SymbolTable::get_generic_struct(const std::string &name) {
    return get_symbol<GenericStruct, &SymbolTable::generic_structs>(name);
}

void SymbolTable::add_generic_struct(GenericStruct *generic_struct) {
    generic_structs.push_back(generic_struct);
    symbols.insert({generic_struct->get_name(), generic_struct});
}

std::optional<SymbolRef> SymbolTable::get_symbol(const std::string &name) {
    auto it = symbols.find(name);
    if (it == symbols.end() || it->second.get_kind() == SymbolKind::FIELD) {
        return parent ? parent->get_symbol(name) : std::optional<SymbolRef>();
    } else {
        return it->second.resolve();
    }
}

std::optional<SymbolRef> SymbolTable::get_symbol_unresolved(const std::string &name) {
    auto it = symbols.find(name);
    if (it == symbols.end() || it->second.get_kind() == SymbolKind::FIELD) {
        return parent ? parent->get_symbol_unresolved(name) : std::optional<SymbolRef>();
    } else {
        return it->second;
    }
}

void SymbolTable::add_symbol(std::string name, SymbolRef symbol) {
    symbols.insert({std::move(name), symbol});
}

void SymbolTable::add_groupable_symbol(std::string name, SymbolRef symbol) {
    auto it = symbols.find(name);
    if (it == symbols.end()) {
        symbols.insert({std::move(name), symbol});
    } else if (it->second.get_kind() == SymbolKind::GROUP) {
        it->second.get_group()->symbols.push_back(symbol);
    } else {
        SymbolGroup *group = new SymbolGroup();
        group->symbols = {it->second, symbol};
        it->second = group;
    }
}

void SymbolTable::replace_symbol(const std::string &name, SymbolRef symbol) {
    auto iter = symbols.find(name);
    if (iter == symbols.end()) {
        symbols.insert({name, symbol});
    } else {
        iter->second.destroy();
        iter->second = symbol;
    }
}

bool SymbolTable::does_param_list_match(const std::vector<DataType *> &a, const std::vector<DataType *> &b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (int i = 0; i < a.size(); i++) {
        if (!a[i]->equals(b[i])) {
            return false;
        }
    }

    return true;
}

} // namespace lang

} // namespace banjo
