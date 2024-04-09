#ifndef ENUMERATION_H
#define ENUMERATION_H

#include "symbol/symbol.hpp"
#include "symbol/symbol_table.hpp"
#include "utils/large_int.hpp"

#include <string>
#include <utility>

namespace lang {

class ASTModule;
class Enumeration;

class EnumVariant : public Symbol {

private:
    LargeInt value;
    Enumeration *enum_;

public:
    EnumVariant();
    EnumVariant(ASTNode *node, std::string name, LargeInt value, Enumeration *enum_);

    const LargeInt &get_value() const { return value; }
    Enumeration *get_enum() const { return enum_; }
};

class Enumeration : public Symbol {

private:
    ASTModule *module_;
    SymbolTable symbol_table;

public:
    Enumeration();
    Enumeration(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *parent_symbol_table);

    ASTModule *get_module() const { return module_; }
    SymbolTable *get_symbol_table() { return &symbol_table; }
};

} // namespace lang

#endif
