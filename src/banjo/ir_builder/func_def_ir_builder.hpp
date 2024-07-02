#ifndef IR_BUILDER_FUNC_DEF_IR_BUILDER_H
#define IR_BUILDER_FUNC_DEF_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder.hpp"

namespace banjo {

namespace ir_builder {

class FuncDefIRBuilder : public IRBuilder {

public:
    FuncDefIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();
    void build_closure(lang::Function *lang_func);

private:
    void build(lang::Function *lang_func, lang::ASTNode *block_node, std::string name);
    void create_func(lang::Function *lang_func, std::string name);
    void init_lang_params(lang::Function *lang_func, std::vector<lang::Parameter *> &lang_params);
    void build_arg_store(ir::Function *func);
    void build_block(lang::ASTNode *block);
    void build_return();

    bool has_return_value();
    bool is_external(lang::Function *func);
};

} // namespace ir_builder

} // namespace banjo

#endif
