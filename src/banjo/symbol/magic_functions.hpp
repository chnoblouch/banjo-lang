#ifndef MAGIC_FUNCTIONS_H
#define MAGIC_FUNCTIONS_H

#include "banjo/ast/ast_node.hpp"
#include <string>

namespace banjo {

namespace lang {

namespace MagicFunctions {

const std::string DEINIT = "__deinit__";
const std::string ADD = "__add__";
const std::string SUB = "__sub__";
const std::string MUL = "__mul__";
const std::string DIV = "__div__";
const std::string EQ = "__eq__";
const std::string NE = "__ne__";
const std::string ITER = "__iter__";
const std::string NEXT = "__next__";

const std::string &get_operator_func(ASTNodeType type);

} // namespace MagicFunctions

} // namespace lang

} // namespace banjo

#endif
