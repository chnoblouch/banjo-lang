#ifndef IR_BUILDER_EXPR_IR_BUILDER_H
#define IR_BUILDER_EXPR_IR_BUILDER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/ir_builder.hpp"
#include "banjo/ir_builder/storage.hpp"
#include "banjo/symbol/data_type.hpp"

namespace banjo {

namespace ir_builder {

class ExprIRBuilder : public IRBuilder {

private:
    lang::DataType *lang_type;
    unsigned coercion_level = 0;

public:
    ExprIRBuilder(IRBuilderContext &context, lang::ASTNode *node);
    void set_coercion_level(unsigned coercion_level);

    StoredValue build();
    StoredValue build_into_value();
    StoredValue build_into_value_if_possible();
    StoredValue build_into_ptr();
    void build_and_store(const ir::Value &dst);
    void build_and_store(ir::VirtualRegister dst);

private:
    StoredValue build_node(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_int_literal(lang::ASTNode *node);
    StoredValue build_float_literal(lang::ASTNode *node);
    StoredValue build_char_literal(lang::ASTNode *node);
    StoredValue build_string_literal(lang::ASTNode *node);
    StoredValue build_array_literal(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_dynamic_array_literal(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_static_array_literal(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_map_literal(lang::ASTNode *node);
    StoredValue build_struct_literal(lang::ASTNode *node, unsigned values_index, const StorageHints &hints);
    StoredValue build_tuple_literal(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_binary_operation(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_overloaded_operator_call(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_neg(lang::ASTNode *node);
    StoredValue build_ref(lang::ASTNode *node);
    StoredValue build_deref(lang::Expr *node);
    StoredValue build_not(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_location(lang::ASTNode *node);
    StoredValue build_call(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_union_case_expr(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_func_call(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_bracket_expr(lang::BracketExpr *node);
    StoredValue build_cast(lang::ASTNode *node);
    StoredValue build_meta_expr(lang::ASTNode *node);

    StoredValue build_bool_expr(lang::ASTNode *node, const StorageHints &hints);
    StoredValue build_implicit_optional(lang::Expr *expr);
    StoredValue build_implicit_result(lang::Expr *expr);
    StoredValue build_implicit_proto_ptr(lang::Expr *expr, const StorageHints &hints);
    char encode_char(const std::string &value, unsigned &index);
    StoredValue build_cstr_string_literal(const std::string &value);
    StoredValue build_offset_ptr(const StoredValue &pointer, const StoredValue &index, ir::Type base_type);
    StoredValue create_immediate(LargeInt value, ir::Type type);

    bool is_coerced();
    lang::DataType *get_coercion_base();
    bool is_overloaded_operator_call(lang::ASTNode *node);
};

} // namespace ir_builder

} // namespace banjo

#endif
