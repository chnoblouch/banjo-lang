#ifndef AST_UTILS_H
#define AST_UTILS_H

#include "ast/ast_node.hpp"
#include "symbol/symbol.hpp"
#include "symbol/symbol_table.hpp"

#include <functional>

namespace banjo {

namespace lang {

namespace ASTUtils {

ModulePath get_path_from_node(ASTNode *node);
void append_path_from_node(ASTNode *node, ModulePath &dest);

SymbolTable *find_symbol_table(ASTNode *node);
ASTModule *find_module(ASTNode *node);

SymbolTable *get_module_symbol_table(ASTNode *module_node);
Symbol *get_symbol(ASTNode *node);

bool is_func_method(ASTNode *node);
GenericStruct *resolve_generic_struct(const ModulePath &path, SymbolTable *symbol_table);

void iterate_funcs(lang::SymbolTable *symbol_table, std::function<void(lang::Function *)> callback);
void iterate_structs(lang::SymbolTable *symbol_table, std::function<void(lang::Structure *)> callback);
void iterate_unions(lang::SymbolTable *symbol_table, std::function<void(lang::Union *)> callback);
void iterate_protos(lang::SymbolTable *symbol_table, std::function<void(lang::Protocol *)> callback);

} // namespace ASTUtils

} // namespace lang

} // namespace banjo

#endif
