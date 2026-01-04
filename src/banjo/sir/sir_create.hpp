#ifndef BANJO_SIR_CREATE_H
#define BANJO_SIR_CREATE_H

#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sir {

sir::Expr create_primitive_type(sir::Module &mod, sir::Primitive primitive);
sir::Expr create_pseudo_type(sir::Module &mod, sir::PseudoTypeKind kind);
sir::Expr create_call(sir::Module &mod, sir::FuncDef &func_def, std::span<sir::Expr> args);
sir::Expr create_unary_ref(sir::Module &mod, sir::Expr base_value);
sir::Expr create_error_value(sir::Module &mod, ASTNode *ast_node);
sir::Stmt create_return_result_success_void(sir::Module &mod, sir::StructDef &result_def);

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
