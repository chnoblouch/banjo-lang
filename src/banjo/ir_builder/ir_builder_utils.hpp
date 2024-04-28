#ifndef IR_BUILDER_UTILS_H
#define IR_BUILDER_UTILS_H

#include "ast/ast_node.hpp"
#include "ir/type.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/storage.hpp"
#include "symbol/data_type.hpp"
#include "symbol/global_variable.hpp"
#include "symbol/variable.hpp"

#include <string>
#include <vector>

namespace ir_builder {

namespace IRBuilderUtils {

struct FuncCall {
    lang::Function *func;
    const std::vector<ir::Value> &args;
};

ir::Type build_type(lang::DataType *type);
std::vector<ir::Type> build_params(const std::vector<lang::DataType *> &lang_params, IRBuilderContext &context);
std::string get_func_link_name(lang::Function *func);
std::string get_global_var_link_name(lang::GlobalVariable *global_var);
void copy_val(IRBuilderContext &context, ir::Value src_ptr, ir::Value dst_ptr, ir::Type type);
ir::Value build_arg(lang::ASTNode *node, IRBuilderContext &context);
StoredValue build_call(lang::Function *func, const std::vector<ir::Value> &args, IRBuilderContext &context);
StoredValue build_call(FuncCall call, IRBuilderContext &context);
StoredValue build_call(FuncCall call, ir::Value dst, IRBuilderContext &context);
ir::VirtualRegister get_cur_return_reg(IRBuilderContext &context);
bool is_branch(ir::Instruction &instr);
bool is_branching(ir::BasicBlock &block);

} // namespace IRBuilderUtils

} // namespace ir_builder

#endif
