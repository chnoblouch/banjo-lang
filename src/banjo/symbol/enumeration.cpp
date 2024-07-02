#include "enumeration.hpp"

#include "banjo/symbol/symbol_table.hpp"
#include <utility>

namespace banjo {

namespace lang {

EnumVariant::EnumVariant() : Symbol(nullptr, ""), value(0) {}

EnumVariant::EnumVariant(ASTNode *node, std::string name, LargeInt value, Enumeration *enum_)
  : Symbol(node, std::move(name)),
    value(value),
    enum_(enum_) {}

Enumeration::Enumeration() : Symbol(nullptr, ""), symbol_table(nullptr) {}

Enumeration::Enumeration(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *parent_symbol_table)
  : Symbol(node, std::move(name)),
    module_(module_),
    symbol_table(SymbolTable(parent_symbol_table)) {}

} // namespace lang

} // namespace banjo
