#ifndef BANJO_SIR_CREATE_H
#define BANJO_SIR_CREATE_H

#include "banjo/sir/sir.hpp"

namespace banjo::lang::sir {

Expr create_primitive_type(Module &mod, Primitive primitive);
Expr create_pseudo_type(Module &mod, PseudoTypeKind kind);
CallExpr *create_call(Module &mod, Concrete<FuncDef> concrete_func, std::span<Expr> args);
CallExpr *create_call(Module &mod, FuncDef &func_def, std::span<Expr> args);
Expr create_unary_ref(Module &mod, Expr base_value);
Expr create_error_value(Module &mod, ASTNode *ast_node);
Expr create_result_success_void(Module &mod, Concrete<StructDef> concrete_struct);
Stmt create_return_result_success_void(Module &mod, Concrete<StructDef> concrete_struct);

} // namespace banjo::lang::sir

#endif
