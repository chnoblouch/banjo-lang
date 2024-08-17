#ifndef MAGIC_METHODS_H
#define MAGIC_METHODS_H

#include "banjo/sir/sir.hpp"

#include <string_view>

namespace banjo {

namespace lang {

namespace sema {

namespace MagicMethods {

const std::string_view OP_ADD = "__add__";
const std::string_view OP_SUB = "__sub__";
const std::string_view OP_MUL = "__mul__";
const std::string_view OP_DIV = "__div__";
const std::string_view OP_MOD = "__mod__";
const std::string_view OP_BIT_AND = "__bitand__";
const std::string_view OP_BIT_OR = "__bitor__";
const std::string_view OP_BIT_XOR = "__bitxor__";
const std::string_view OP_SHL = "__shl__";
const std::string_view OP_SHR = "__shr__";
const std::string_view OP_EQ = "__eq__";
const std::string_view OP_NE = "__ne__";
const std::string_view OP_GT = "__gt__";
const std::string_view OP_LT = "__lt__";
const std::string_view OP_GE = "__ge__";
const std::string_view OP_LE = "__le__";
const std::string_view OP_NEG = "__neg__";
const std::string_view OP_DEREF = "__deref__";
const std::string_view OP_INDEX = "__index__";

std::string_view look_up(sir::BinaryOp op);
std::string_view look_up(sir::UnaryOp op);

} // namespace MagicMethods

} // namespace sema2

} // namespace lang

} // namespace banjo

#endif
