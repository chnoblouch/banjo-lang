#ifndef AST_BLOCK_H
#define AST_BLOCK_H

#include "ast/ast_node.hpp"
#include "symbol/symbol_table.hpp"

#include <vector>

namespace banjo {

namespace lang {

class SymbolTable;
struct DeinitInfo;

struct ValueMove {
    ASTNode *node;
    DeinitInfo *deinit_info;
};

class ASTBlock : public ASTNode {

private:
    SymbolTable symbol_table;
    std::vector<DeinitInfo *> deinit_values;
    std::vector<ValueMove> value_moves;

public:
    ASTBlock();
    ASTBlock(TextRange range, SymbolTable *parent_symbol_table);
    ASTNode *create_clone();
    ASTBlock *clone_block(SymbolTable *parent_symbol_table);

    SymbolTable *get_symbol_table() { return &symbol_table; }
    const std::vector<DeinitInfo *> &get_deinit_values() { return deinit_values; }
    std::vector<ValueMove> &get_value_moves() { return value_moves; }

    void set_parent_symbol_table(SymbolTable *parent_symbol_table);
    void add_deinit_value(DeinitInfo *deinit_value) { deinit_values.push_back(deinit_value); }
    void add_value_move(ValueMove value_move) { value_moves.push_back(value_move); }

private:
    ASTNode *clone_block_child(ASTNode *child, SymbolTable *parent_symbol_table);
};

} // namespace lang

} // namespace banjo

#endif
