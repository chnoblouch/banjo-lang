#ifndef BANJO_AST_MODULE_H
#define BANJO_AST_MODULE_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/reports/report.hpp"
#include "banjo/utils/typed_arena.hpp"

#include <span>
#include <vector>

namespace banjo::lang {

class SourceFile;

class ASTModule : public ASTNode {

public:
    SourceFile &file;
    bool is_valid;
    std::vector<Report> reports;

private:
    utils::TypedArena<ASTNode, 128> node_arena;
    utils::Arena<256> token_index_arena;

public:
    ASTModule(SourceFile &file) : ASTNode(AST_MODULE), file(file), is_valid(true) {}

    template <typename... Args>
    ASTNode *create_node(Args... args) {
        return node_arena.create(args...);
    }

    std::span<unsigned> create_token_index(unsigned index) {
        return std::span<unsigned>{token_index_arena.create<unsigned>(index), 1};
    }

    std::span<unsigned> create_token_indices(std::vector<unsigned> &indices) {
        return token_index_arena.create_array(std::span<unsigned>{indices.begin(), indices.end()});
    }

    ASTNode *get_block() { return first_child; }
};

} // namespace banjo::lang

#endif
