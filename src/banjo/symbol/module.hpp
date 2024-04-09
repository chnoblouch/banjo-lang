#ifndef MODULE_H
#define MODULE_H

#include "symbol/module_path.hpp"

namespace lang {

class ASTNode;

class Module {

private:
    ASTNode *node;
    ModulePath path;

public:
    Module(ASTNode *node, ModulePath path) : node(node), path(path) {}
    ASTNode *get_node() { return node; }
    const ModulePath &get_path() { return path; }
};

} // namespace lang

#endif
