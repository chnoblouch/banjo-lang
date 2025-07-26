#ifndef BANJO_AST_WRITER_H
#define BANJO_AST_WRITER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/module_list.hpp"

#include <ostream>

namespace banjo {

namespace lang {

class ASTWriter {

private:
    std::ostream &stream;

public:
    ASTWriter(std::ostream &stream);
    void write(ASTNode *node);
    void write_all(ModuleList &module_list);

private:
    void write(ASTNode *node, unsigned indentation);
    const char *get_type_name(ASTNodeType type);
};

} // namespace lang

} // namespace banjo

#endif
