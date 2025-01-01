#ifndef AST_MODULE_H
#define AST_MODULE_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/utils/growable_arena.hpp"

#include <filesystem>
#include <vector>

namespace banjo {

namespace lang {

class ASTModule : public ASTNode {

private:
    ModulePath path;
    std::filesystem::path file_path;
    std::vector<ASTModule *> sub_mods;
    utils::GrowableArena<ASTNode, 128> node_arena;

public:
    ASTModule(ModulePath path) : ASTNode(AST_MODULE), path(path) {}

    const ModulePath &get_path() { return path; }
    const std::filesystem::path &get_file_path() { return file_path; }
    const std::vector<ASTModule *> &get_sub_mods() { return sub_mods; }

    void set_file_path(std::filesystem::path file_path) { this->file_path = file_path; }
    void add_sub_mod(ASTModule *sub_mod) { sub_mods.push_back(sub_mod); }

    template <typename... Args>
    ASTNode *create_node(Args... args) {
        return node_arena.create(args...);
    }

    ASTNode *get_block() { return first_child; }
};

} // namespace lang

} // namespace banjo

#endif
