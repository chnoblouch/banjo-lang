#include "magic_functions.hpp"

#include "utils/macros.hpp"

namespace banjo {

namespace lang {

const std::string &MagicFunctions::get_operator_func(ASTNodeType type) {
    switch (type) {
        case AST_OPERATOR_ADD: return ADD;
        case AST_OPERATOR_SUB: return SUB;
        case AST_OPERATOR_MUL: return MUL;
        case AST_OPERATOR_DIV: return DIV;
        case AST_OPERATOR_EQ: return EQ;
        case AST_OPERATOR_NE: return NE;
        default: ASSERT_UNREACHABLE;
    }
}

} // namespace lang

} // namespace banjo
