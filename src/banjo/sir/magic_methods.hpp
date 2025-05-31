#ifndef BANJO_SIR_MAGIC_METHODS_H
#define BANJO_SIR_MAGIC_METHODS_H

#include "banjo/sir/sir.hpp"

#include <string_view>

namespace banjo {

namespace lang {

namespace sir {

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
const std::string_view OP_BIT_NOT = "__bitnot__";
const std::string_view OP_DEREF = "__deref__";
const std::string_view OP_INDEX = "__index__";
const std::string_view OP_MUT_INDEX = "__mutindex__";

const std::string_view ITER = "__iter__";
const std::string_view REF_ITER = "__refiter__";
const std::string_view MUT_ITER = "__mutiter__";
const std::string_view NEXT = "__next__";

const std::string_view DEINIT = "__deinit__";

std::string_view look_up(sir::BinaryOp op);
std::string_view look_up(sir::UnaryOp op);
std::string_view look_up_iter(sir::IterKind kind);

} // namespace MagicMethods

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
