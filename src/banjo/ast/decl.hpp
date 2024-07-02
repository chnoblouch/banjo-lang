#ifndef DECL_H
#define DECL_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/symbol/enumeration.hpp"
#include "banjo/symbol/generics.hpp"
#include "banjo/symbol/protocol.hpp"
#include "banjo/symbol/structure.hpp"
#include "banjo/symbol/type_alias.hpp"
#include "banjo/symbol/union.hpp"


#include <utility>

namespace banjo {

namespace lang {

class Function;
class Constant;
class Variable;
class Structure;

template <typename T, ASTNodeType SymbolType>
class ASTSymbol : public ASTNode {

private:
    T *symbol;

public:
    ASTSymbol(T *symbol = nullptr) : ASTNode(SymbolType), symbol(symbol) {}
    T *get_symbol() const { return symbol; }
    void set_symbol(T *symbol) { this->symbol = symbol; }
    ASTNode *create_clone() override { return new ASTSymbol(symbol); }
};

template <typename T, ASTNodeType SymbolType>
class ASTOwningSymbol : public ASTNode {

private:
    T symbol;

public:
    ASTOwningSymbol(T symbol = {}) : ASTNode(SymbolType), symbol(std::move(symbol)) {}
    T *get_symbol() { return &symbol; }
    void set_symbol(T symbol) { this->symbol = std::move(symbol); }
    ASTNode *create_clone() override { return new ASTOwningSymbol(symbol); }
};

class ASTFunc : public ASTNode {

private:
    Function symbol;

public:
    ASTFunc(ASTNodeType type, Function symbol = {}) : ASTNode(type), symbol(std::move(symbol)) {}

    Function *get_symbol() { return &symbol; }
    void set_symbol(Function symbol) { this->symbol = std::move(symbol); }
    ASTNode *create_clone() override { return new ASTFunc(type, symbol); }
};

class ASTVar : public ASTNode {

private:
    Symbol *symbol;

public:
    ASTVar(ASTNodeType type, Symbol *symbol = nullptr) : ASTNode(type), symbol(symbol) {}
    Symbol *get_symbol() const { return symbol; }
    void set_symbol(Symbol *symbol) { this->symbol = symbol; }
    ASTNode *create_clone() override { return new ASTVar(type, nullptr); }
};

typedef ASTOwningSymbol<Constant, AST_CONSTANT> ASTConst;
typedef ASTOwningSymbol<Structure, AST_STRUCT_DEFINITION> ASTStruct;
typedef ASTOwningSymbol<Enumeration, AST_ENUM_DEFINITION> ASTEnum;
typedef ASTOwningSymbol<EnumVariant, AST_ENUM_VARIANT> ASTEnumVariant;
typedef ASTOwningSymbol<Union, AST_UNION> ASTUnion;
typedef ASTOwningSymbol<UnionCase, AST_UNION_CASE> ASTUnionCase;
typedef ASTOwningSymbol<Protocol, AST_PROTO> ASTProto;
typedef ASTOwningSymbol<TypeAlias, AST_TYPE_ALIAS> ASTTypeAlias;
typedef ASTOwningSymbol<GenericFunc, AST_GENERIC_FUNCTION_DEFINITION> ASTGenericFunc;
typedef ASTOwningSymbol<GenericStruct, AST_GENERIC_STRUCT_DEFINITION> ASTGenericStruct;

} // namespace lang

} // namespace banjo

#endif
