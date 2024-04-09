#ifndef IR_BUILDER_EXPR_IR_BUILDER_H
#define IR_BUILDER_EXPR_IR_BUILDER_H

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "ir/operand.hpp"
#include "ir_builder/ir_builder.hpp"
#include "ir_builder/storage.hpp"
#include "symbol/data_type.hpp"

namespace ir_builder {

class ExprIRBuilder : public IRBuilder {

private:
    lang::DataType *lang_type;
    unsigned coercion_level = 0;

public:
    ExprIRBuilder(IRBuilderContext &context, lang::ASTNode *node);
    void set_coercion_level(unsigned coercion_level);

    StoredValue build(StorageReqs reqs);
    StoredValue build_into_stored_val_if_possible(StorageReqs reqs = {});
    ir::Value build_into_value();
    ir::Value build_into_value_if_possible();
    ir::Value build_into_ptr();
    void build_and_store(StorageReqs reqs);

private:
    ir::Value build_into_value(lang::ASTNode *node);

    StoredValue build_node(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_int_literal(lang::ASTNode *node);
    StoredValue build_float_literal(lang::ASTNode *node);
    StoredValue build_char_literal(lang::ASTNode *node);
    StoredValue build_string_literal(lang::ASTNode *node);
    StoredValue build_array_literal(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_map_literal(lang::ASTNode *node);
    StoredValue build_struct_literal(lang::ASTNode *node, unsigned values_index, StorageReqs reqs);
    StoredValue build_tuple_literal(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_binary_operation(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_overloaded_operator_call(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_neg(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_ref(lang::ASTNode *node);
    StoredValue build_deref(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_not(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_location(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_call(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_union_case_expr(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_func_call(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_bracket_expr(lang::BracketExpr *node, StorageReqs reqs);
    StoredValue build_cast(lang::ASTNode *node);
    StoredValue build_meta_expr(lang::ASTNode *node);

    StoredValue build_bool_expr(lang::ASTNode *node, StorageReqs reqs);
    StoredValue build_implicit_optional(lang::Expr *expr, StorageReqs reqs);
    StoredValue build_implicit_result(lang::Expr *expr, StorageReqs reqs);
    char encode_char(const std::string &value, unsigned &index);
    StoredValue build_cstr_string_literal(const std::string &value);
    StoredValue build_offset_ptr(StoredValue pointer, StoredValue index);
    StoredValue create_immediate(LargeInt value, ir::Type type);

    bool is_coerced();
    lang::DataType *get_coercion_base();
    bool is_overloaded_operator_call(lang::ASTNode *node);
};

} // namespace ir_builder

#endif
