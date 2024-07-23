#ifndef AST_MODULE_H
#define AST_MODULE_H

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/symbol/module_path.hpp"

#include <filesystem>
#include <vector>

namespace banjo {

namespace lang {

class ASTModule : public ASTNode {

private:
    ModulePath path;
    std::filesystem::path file_path;
    std::vector<ASTModule *> sub_mods;
    std::vector<ASTModule *> dependents;

public:
    ASTModule(ModulePath path) : ASTNode(AST_MODULE), path(path) {}

    const ModulePath &get_path() { return path; }
    const std::filesystem::path &get_file_path() { return file_path; }
    const std::vector<ASTModule *> &get_sub_mods() { return sub_mods; }
    const std::vector<ASTModule *> &get_dependents() { return dependents; }

    void set_file_path(std::filesystem::path file_path) { this->file_path = file_path; }
    void add_sub_mod(ASTModule *sub_mod) { sub_mods.push_back(sub_mod); }
    void add_dependent(ASTModule *dependent) { dependents.push_back(dependent); }

    ASTNode *create_clone() override { return new ASTModule(path); }

    ASTBlock *get_block() { return get_child_of_type(AST_BLOCK)->as<ASTBlock>(); }
};

} // namespace lang

} // namespace banjo

#endif
