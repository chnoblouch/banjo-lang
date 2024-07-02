#ifndef MODULE_H
#define MODULE_H

#include "banjo/symbol/module_path.hpp"

namespace banjo {

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

} // namespace banjo

#endif
