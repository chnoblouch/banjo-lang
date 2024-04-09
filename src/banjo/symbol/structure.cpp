#include "structure.hpp"

#include "symbol/symbol.hpp"
#include "symbol/symbol_table.hpp"
#include <utility>

namespace lang {

StructField::StructField() : Symbol(nullptr, "") {}

StructField::StructField(ASTNode *node, std::string name, ASTModule *module_)
  : Symbol(node, std::move(name)),
    module_(module_) {}

Structure::Structure() : Symbol(nullptr, "") {}

Structure::Structure(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *symbol_table)
  : Symbol(node, std::move(name)),
    module_(module_),
    symbol_table(symbol_table) {}

Structure::Structure(const Structure &other)
  : Symbol(other.node, other.name),
    module_(other.module_),
    symbol_table(other.symbol_table),
    generic_params(other.generic_params),
    generic_instances(other.generic_instances) {
    for (StructField *field : other.fields) {
        fields.push_back(field);
    }
}

Structure::Structure(Structure &&other) noexcept
  : Symbol(other.node, other.name),
    module_(other.module_),
    fields(std::exchange(other.fields, {})),
    symbol_table(other.symbol_table),
    generic_params(std::exchange(other.generic_params, {})),
    generic_instances(std::exchange(other.generic_instances, {})) {}

Structure::~Structure() {
    for (StructField *field : fields) {
        delete field;
    }
}

Structure &Structure::operator=(const Structure &other) {
    node = other.node;
    name = other.name;
    module_ = other.module_;
    symbol_table = other.symbol_table;
    generic_params = other.generic_params;
    generic_instances = other.generic_instances;

    for (StructField *field : other.fields) {
        fields.push_back(field);
    }

    return *this;
}

Structure &Structure::operator=(Structure &&other) noexcept {
    node = other.node;
    name = other.name;
    module_ = other.module_;
    fields = std::exchange(other.fields, {});
    symbol_table = other.symbol_table;
    generic_params = std::exchange(other.generic_params, {});
    generic_instances = std::exchange(other.generic_instances, {});
    return *this;
}

StructField *Structure::get_field(std::string_view name) {
    for (StructField *field : fields) {
        if (field->get_name() == name) {
            return field;
        }
    }

    return nullptr;
}

int Structure::get_field_index(StructField *field) {
    for (int i = 0; i < fields.size(); i++) {
        if (fields[i] == field) {
            return i;
        }
    }

    return -1;
}

} // namespace lang
