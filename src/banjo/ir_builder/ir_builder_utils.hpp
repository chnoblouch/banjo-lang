#ifndef IR_BUILDER_UTILS_H
#define IR_BUILDER_UTILS_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ir/type.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/storage.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/global_variable.hpp"
#include "banjo/symbol/variable.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace ir_builder {

namespace IRBuilderUtils {

struct FuncCall {
    lang::Function *func;
    const std::vector<ir::Value> &args;
};

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

} // namespace banjo

#endif
