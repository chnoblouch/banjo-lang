#include "report_generator.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/reports/report.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>

namespace banjo {

namespace lang {

namespace sema {

ReportBuilder::ReportBuilder(SemanticAnalyzer &analyzer, Report::Type type)
  : analyzer(analyzer),
    partial_report(type) {}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::set_message(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    partial_report.set_message({find_location(node), format_str, format_args...});
    return *this;
}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::add_note(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    partial_report.add_note({find_location(node), format_str, format_args...});
    return *this;
}

SourceLocation ReportBuilder::find_location(ASTNode *node) {
    if (node) {
        return SourceLocation{find_mod_path(node), node->range};
    } else {
        return SourceLocation{{}, {0, 0}};
    }
}

void ReportBuilder::report() {
    // std::unordered_map<std::string_view, sir::Expr> &generic_args = analyzer.get_scope().generic_args;

    // if (!generic_args.empty()) {
    //     for (auto &[name, value] : generic_args) {
    //         add_note("generic argument '$' = '$'", value.get_ast_node(), name, value);
    //     }
    // }

    analyzer.report_manager.insert(partial_report);
}

const ModulePath &ReportBuilder::find_mod_path(ASTNode *node) {
    while (node->type != AST_MODULE) {
        node = node->parent;
    }

    return static_cast<ASTModule *>(node)->get_path();
}

ReportGenerator::ReportGenerator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

template <typename... FormatArgs>
ReportBuilder ReportGenerator::build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    return ReportBuilder(analyzer, Report::Type::ERROR).set_message(format_str, node, format_args...);
}

template <typename... FormatArgs>
ReportBuilder ReportGenerator::build_warning(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    return ReportBuilder(analyzer, Report::Type::WARNING).set_message(format_str, node, format_args...);
}

template <typename... FormatArgs>
void ReportGenerator::report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    build_error(format_str, node, format_args...).report();
}

template <typename... FormatArgs>
void ReportGenerator::report_warning(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    build_warning(format_str, node, format_args...).report();
}

void ReportGenerator::report_err_expr_category(const sir::Expr &expr, sir::ExprCategory expected) {
    report_error("expected $, got $", expr.get_ast_node(), expected, expr.get_category());
}

void ReportGenerator::report_err_symbol_not_found(const sir::IdentExpr &ident_expr) {
    report_error("cannot find '$'", ident_expr.ast_node, ident_expr.value);
}

void ReportGenerator::report_err_symbol_not_found(const sir::Ident &ident) {
    report_error("cannot find '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_symbol_not_found_in(const sir::Ident &member_ident, const std::string &base_name) {
    report_error("cannot find '$' in '$'", member_ident.ast_node, member_ident.value, base_name);
}

void ReportGenerator::report_err_module_not_found(const sir::Ident &ident) {
    report_error("cannot find module '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_redefinition(const sir::Ident &ident, const sir::Symbol &prev_def) {
    build_error("redefinition of '$'", ident.ast_node, ident.value)
        .add_note("previously defined here", prev_def.get_ident().ast_node)
        .report();
}

void ReportGenerator::report_err_type_mismatch(
    const sir::Expr &value,
    const sir::Expr &expected,
    const sir::Expr &actual
) {
    report_error("type mismatch (expected '$', got '$')", value.get_ast_node(), expected, actual);
}

void ReportGenerator::report_err_cannot_coerce(const sir::Expr &expr, const sir::Expr &expected_type) {
    report_error("cannot coerce value with type '$' to type '$'", expr.get_ast_node(), expr.get_type(), expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::IntLiteral &int_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce int literal to type '$'", int_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::FPLiteral &fp_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce float literal to type '$'", fp_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::NullLiteral &null_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce `null` to type '$'", null_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::NoneLiteral &none_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce `none` to type '$'", none_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::ArrayLiteral &array_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce array literal to type '$'", array_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(
    const sir::StringLiteral &string_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce string literal to type '$'", string_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(
    const sir::StructLiteral &struct_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce struct literal to type '$'", struct_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce(const sir::MapLiteral &map_literal, const sir::Expr &expected_type) {
    report_error("cannot coerce map literal to type '$'", map_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cannot_coerce_result(
    const sir::Expr &value,
    sir::Specialization<sir::StructDef> &result_specialization
) {
    report_error(
        "cannot coerce value with type '$' to result type '$ except $'",
        value.get_ast_node(),
        value.get_type(),
        result_specialization.args[0],
        result_specialization.args[1]
    );
}

void ReportGenerator::report_err_ref_immut_to_mut(const sir::Expr &expr, const sir::Expr &immut_sub_expr) {
    ReportBuilder builder =
        build_error("cannot pass immutable by mutable reference ('ref mut $')", expr.get_ast_node(), expr.get_type());

    add_immut_sub_expr_note(builder, immut_sub_expr);
    builder.report();
}

void ReportGenerator::report_err_cannot_infer_type(const sir::NoneLiteral &none_literal) {
    report_error("cannot infer type of `none`", none_literal.ast_node);
}

void ReportGenerator::report_err_cannot_infer_type(const sir::UndefinedLiteral &undefined_literal) {
    report_error("cannot infer type of `undefined`", undefined_literal.ast_node);
}

void ReportGenerator::report_err_cannot_infer_type(const sir::ArrayLiteral &array_literal) {
    report_error("cannot infer type of empty array literal", array_literal.ast_node);
}

void ReportGenerator::report_err_cannot_infer_type(const sir::StructLiteral &struct_literal) {
    report_error("cannot infer type of struct literal", struct_literal.ast_node);
}

void ReportGenerator::report_err_cannot_infer_lhs(const sir::DotExpr &dot_expr) {
    report_error("cannot infer left-hand side", dot_expr.ast_node);
}

void ReportGenerator::report_err_cannot_infer_lhs(const sir::DotExpr &dot_expr, sir::Expr type) {
    report_error("cannot infer left-hand side (type is '$' instead of an enum)", dot_expr.ast_node, type);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::BinaryExpr &binary_expr) {
    std::string_view operator_name;

    switch (binary_expr.op) {
        case sir::BinaryOp::ADD: operator_name = "+"; break;
        case sir::BinaryOp::SUB: operator_name = "-"; break;
        case sir::BinaryOp::MUL: operator_name = "*"; break;
        case sir::BinaryOp::DIV: operator_name = "/"; break;
        case sir::BinaryOp::MOD: operator_name = "%"; break;
        case sir::BinaryOp::BIT_AND: operator_name = "&"; break;
        case sir::BinaryOp::BIT_OR: operator_name = "|"; break;
        case sir::BinaryOp::BIT_XOR: operator_name = "^"; break;
        case sir::BinaryOp::SHL: operator_name = "<<"; break;
        case sir::BinaryOp::SHR: operator_name = ">>"; break;
        case sir::BinaryOp::EQ: operator_name = "=="; break;
        case sir::BinaryOp::NE: operator_name = "!="; break;
        case sir::BinaryOp::GT: operator_name = ">"; break;
        case sir::BinaryOp::LT: operator_name = "<"; break;
        case sir::BinaryOp::GE: operator_name = ">="; break;
        case sir::BinaryOp::LE: operator_name = "<="; break;
        default: ASSERT_UNREACHABLE;
    }

    std::string_view impl_name = sir::MagicMethods::look_up(binary_expr.op);
    report_err_operator_overload_not_found(binary_expr.ast_node, binary_expr.lhs.get_type(), operator_name, impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::UnaryExpr &unary_expr) {
    std::string_view operator_name;

    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: operator_name = "-"; break;
        case sir::UnaryOp::BIT_NOT: operator_name = "~"; break;
        default: ASSERT_UNREACHABLE;
    }

    std::string_view impl_name = sir::MagicMethods::look_up(unary_expr.op);
    report_err_operator_overload_not_found(unary_expr.ast_node, unary_expr.value.get_type(), operator_name, impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::StarExpr &star_expr) {
    std::string_view impl_name = sir::MagicMethods::look_up(sir::UnaryOp::DEREF);
    report_err_operator_overload_not_found(star_expr.ast_node, star_expr.value.get_type(), "*", impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::BracketExpr &bracket_expr, bool is_mutable) {
    std::optional<std::string_view> second_impl_name;

    if (is_mutable) {
        second_impl_name = sir::MagicMethods::OP_MUT_INDEX;
    }

    report_err_operator_overload_not_found(
        bracket_expr.ast_node,
        bracket_expr.lhs.get_type(),
        "[]",
        sir::MagicMethods::OP_INDEX,
        second_impl_name
    );
}

void ReportGenerator::report_err_cannot_cast(const sir::CastExpr &cast_expr) {
    report_error("cannot cast from '$' to '$'", cast_expr.ast_node, cast_expr.value.get_type(), cast_expr.type);
}

void ReportGenerator::report_err_cannot_call(const sir::Expr &expr) {
    report_error("cannot call value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_deref(const sir::Expr &expr) {
    report_error("cannot dereference value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_assign_immut(const sir::Expr &expr, const sir::Expr &immut_sub_expr) {
    ReportBuilder builder = build_error("cannot assign to immutable", expr.get_ast_node());
    add_immut_sub_expr_note(builder, immut_sub_expr);
    builder.report();
}

void ReportGenerator::report_err_cannot_create_pointer_to_immut(
    const sir::Expr &expr,
    const sir::Expr &immut_sub_expr
) {
    ReportBuilder builder = build_error("cannot create pointer to immutable", expr.get_ast_node());
    add_immut_sub_expr_note(builder, immut_sub_expr);
    builder.report();
}

void ReportGenerator::report_err_cannot_iter(const sir::Expr &expr) {
    report_error("cannot iterate over value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_iter_struct(const sir::Expr &expr, sir::IterKind kind) {
    std::string note_format_str = "implement '$' for this type to support ";

    switch (kind) {
        case sir::IterKind::MOVE: note_format_str += "iteration"; break;
        case sir::IterKind::REF: note_format_str += "reference iteration"; break;
        case sir::IterKind::MUT: note_format_str += "mutating iteration"; break;
    }

    sir::StructDef &struct_def = expr.get_type().as_symbol<sir::StructDef>();

    build_error("cannot iterate over value with type '$'", expr.get_ast_node(), expr.get_type())
        .add_note(note_format_str, struct_def.ident.ast_node, sir::MagicMethods::look_up_iter(kind))
        .report();
}

void ReportGenerator::report_err_iter_no_next(
    const sir::Expr &expr,
    const sir::FuncDef &iter_func_def,
    sir::IterKind kind
) {
    sir::Expr iter_type = iter_func_def.type.return_type;

    std::string note_format_str = "iterator type for ";

    switch (kind) {
        case sir::IterKind::MOVE: note_format_str += "iteration"; break;
        case sir::IterKind::REF: note_format_str += "reference iteration"; break;
        case sir::IterKind::MUT: note_format_str += "mutating iteration"; break;
    }

    note_format_str += " over '$' defined here";

    build_error("iterator type '$' does not implement '$'", expr.get_ast_node(), iter_type, sir::MagicMethods::NEXT)
        .add_note(note_format_str, iter_type.get_ast_node(), expr.get_type())
        .report();
}

void ReportGenerator::report_err_expected_integer(const sir::Expr &expr) {
    report_error("expected integer, got '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_expected_bool(const sir::Expr &expr) {
    // TODO: Maybe add notes e.g. for ints that have to be converted to bools first?

    report_error("expected 'bool', got '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_expected_generic_or_indexable(sir::Expr &expr) {
    report_error("expected generic declaration or indexable value", expr.get_ast_node());
}

void ReportGenerator::report_err_unexpected_arg_count(
    sir::CallExpr &call_expr,
    unsigned expected_count,
    sir::FuncDef *func_def
) {
    std::string format_str;

    if (call_expr.args.size() < expected_count) {
        format_str = "too few arguments (expected $, got $)";
    } else if (call_expr.args.size() > expected_count) {
        format_str = "too many arguments (expected $, got $)";
    }

    ReportBuilder builder = build_error(format_str, call_expr.ast_node, expected_count, call_expr.args.size());

    if (func_def) {
        builder.add_note("function declared with type '$'", func_def->ident.ast_node, sir::Expr(&func_def->type));
    }

    return builder.report();
}

void ReportGenerator::report_err_no_members(const sir::DotExpr &dot_expr) {
    report_error("type '$' doesn't have members", dot_expr.ast_node, dot_expr.lhs.get_type());
}

void ReportGenerator::report_err_no_field(const sir::Ident &field_ident, const sir::StructDef &struct_def) {
    report_error(
        "struct '$' has no field named '$'",
        field_ident.ast_node,
        struct_def.ident.ast_node,
        field_ident.value
    );
}

void ReportGenerator::report_err_no_field(const sir::Ident &field_ident, const sir::UnionCase &union_case) {
    report_error(
        "union case '$' has no field named '$'",
        field_ident.ast_node,
        union_case.ident.ast_node,
        field_ident.value
    );
}

void ReportGenerator::report_err_no_field(const sir::Ident &field_ident, sir::TupleExpr &tuple_expr) {
    report_error("tuple '$' has no field named '$'", field_ident.ast_node, sir::Expr(&tuple_expr), field_ident.value);
}

void ReportGenerator::report_err_missing_field(
    const sir::StructLiteral &struct_literal,
    const sir::StructField &field
) {
    report_error(
        "missing value for field '$' of struct '$'",
        struct_literal.ast_node,
        field.ident.value,
        struct_literal.type.as_symbol<sir::StructDef>().ident.value
    );
}

void ReportGenerator::report_err_duplicate_field(
    const sir::StructLiteral &struct_literal,
    const sir::StructLiteralEntry &entry,
    const sir::StructLiteralEntry &previous_entry
) {
    // TODO: Set range to entire entry.

    build_error(
        "more than one value specified for field '$' of struct '$'",
        entry.ident.ast_node,
        entry.field->ident.value,
        struct_literal.type.as_symbol<sir::StructDef>().ident.value
    )
        .add_note("value first specified here", previous_entry.ident.ast_node)
        .report();
}

void ReportGenerator::report_err_struct_overlapping_not_one_field(const sir::StructLiteral &struct_literal) {

    const sir::StructDef &struct_def = struct_literal.type.as_symbol<sir::StructDef>();

    std::string_view format_str;
    ASTNode *ast_node;

    if (struct_literal.entries.size() == 0) {
        format_str = "no field of struct '$' initialized";
        ast_node = struct_literal.ast_node;
    } else {
        format_str = "more than one field of struct '$' initialized";
        ast_node = struct_literal.entries[1].ident.ast_node; // TODO: Set range to entire entry.
    }

    build_error(format_str, ast_node, struct_def.ident.value)
        .add_note(
            "struct '$' has layout `overlapping` and therefore requires exactly one field value",
            struct_def.ident.ast_node,
            struct_def.ident.value
        )
        .report();
}

void ReportGenerator::report_err_no_method(const sir::Ident &method_ident, const sir::StructDef &struct_def) {
    report_error(
        "struct '$' has no method named '$'",
        method_ident.ast_node,
        struct_def.ident.ast_node,
        method_ident.value
    );
}

void ReportGenerator::report_err_no_matching_overload(const sir::Expr &expr, sir::OverloadSet &overload_set) {
    ReportBuilder builder = build_error("no matching overload found", expr.get_ast_node());

    for (sir::FuncDef *overload : overload_set.func_defs) {
        builder.add_note(
            "type of this candidate does not match: '$'",
            overload->ident.ast_node,
            sir::Expr(&overload->type)
        );
    }

    builder.report();
}

void ReportGenerator::report_err_continue_outside_loop(const sir::ContinueStmt &continue_stmt) {
    report_error("'continue' statement outside of a loop", continue_stmt.ast_node);
}

void ReportGenerator::report_err_break_outside_loop(const sir::BreakStmt &break_stmt) {
    report_error("'break' statement outside of a loop", break_stmt.ast_node);
}

void ReportGenerator::report_err_unexpected_array_length_type(const sir::Expr &expr) {
    report_error("expected integer type, got '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_negative_array_length(const sir::Expr &expr, LargeInt value) {
    report_error("static array length $ is negative", expr.get_ast_node(), value);
}

void ReportGenerator::report_err_unexpected_array_length(
    const sir::ArrayLiteral &array_literal,
    unsigned expected_count
) {
    std::string_view format_str;

    if (array_literal.values.size() < expected_count) {
        format_str = "too few elements in array literal (expected $, got $)";
    } else if (array_literal.values.size() > expected_count) {
        format_str = "too many elements in array literal (expected $, got $)";
    }

    report_error(format_str, array_literal.ast_node, expected_count, array_literal.values.size());
}

void ReportGenerator::report_err_cannot_negate(const sir::UnaryExpr &unary_expr) {
    report_error("cannot negate type '$'", unary_expr.ast_node, unary_expr.value.get_type());
}

void ReportGenerator::report_err_cannot_negate_unsigned(const sir::UnaryExpr &unary_expr) {
    report_error("cannot negate unsigned type '$'", unary_expr.ast_node, unary_expr.value.get_type());
}

void ReportGenerator::report_err_expected_index(const sir::BracketExpr &bracket_expr) {
    // TODO: Range could be just brackets...
    report_error("expected an index", bracket_expr.ast_node);
}

void ReportGenerator::report_err_too_many_indices(const sir::BracketExpr &bracket_expr) {
    report_error("expected just one index, got $", bracket_expr.ast_node, bracket_expr.rhs.size());
}

void ReportGenerator::report_err_unexpected_generic_arg_count(sir::BracketExpr &bracket_expr, sir::FuncDef &func_def) {
    report_err_unexpected_generic_arg_count(bracket_expr, func_def.generic_params, func_def.ident, "function");
}

void ReportGenerator::report_err_unexpected_generic_arg_count(
    sir::BracketExpr &bracket_expr,
    sir::StructDef &struct_def
) {
    report_err_unexpected_generic_arg_count(bracket_expr, struct_def.generic_params, struct_def.ident, "struct");
}

void ReportGenerator::report_err_too_few_args_to_infer_generic_args(const sir::Expr &expr) {
    report_error("too few arguments to infer generic parameter values", expr.get_ast_node());
}

void ReportGenerator::report_err_cannot_infer_generic_arg(
    const sir::Expr &expr,
    const sir::GenericParam &generic_param
) {
    report_error("cannot infer value for generic parameter '$'", expr.get_ast_node(), generic_param.ident.value);
}

void ReportGenerator::report_err_generic_arg_inference_conflict(
    const sir::Expr &expr,
    const sir::GenericParam &generic_param,
    const sir::Expr &first_source,
    const sir::Expr &second_source
) {
    build_error("conflicting values inferred for generic parameter '$'", expr.get_ast_node(), generic_param.ident.value)
        .add_note("first inferred as '$' here", first_source.get_ast_node(), first_source.get_type())
        .add_note("then inferred as '$' here", second_source.get_ast_node(), second_source.get_type())
        .report();
}

void ReportGenerator::report_err_cannot_use_in_try(const sir::Expr &expr) {
    report_error("type '$' is not a result or optional type", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_try_no_error_field(const sir::TryExceptBranch &branch) {
    report_error("optional types don't have an error field", branch.ast_node);
}

void ReportGenerator::report_err_compile_time_unknown(const sir::Expr &value) {
    report_error("value is not known at compile time", value.get_ast_node());
}

void ReportGenerator::report_err_expected_proto(const sir::Expr &expr) {
    report_error("expected proto", expr.get_ast_node());
}

void ReportGenerator::report_err_impl_missing_func(
    const sir::StructDef &struct_def,
    const sir::ProtoFuncDecl &func_decl
) {
    const sir::ProtoDef &proto_def = func_decl.get_parent().as<sir::ProtoDef>();

    build_error(
        "missing implementation of method '$' from proto '$'",
        struct_def.ident.ast_node,
        func_decl.get_ident().value,
        proto_def.ident.value
    )
        .add_note("method declared here", func_decl.get_ident().ast_node)
        .report();
}

void ReportGenerator::report_err_impl_type_mismatch(sir::FuncDef &func_def, sir::ProtoFuncDecl &func_decl) {
    const sir::ProtoDef &proto_def = func_decl.get_parent().as<sir::ProtoDef>();

    build_error(
        "method type does not match declaration (expected '$', got '$')",
        func_def.ident.ast_node,
        sir::Expr(&func_decl.get_type()),
        sir::Expr(&func_def.type)
    )
        .add_note("method declared here, in proto '$'", func_decl.get_ident().ast_node, proto_def.ident.value)
        .report();
}

void ReportGenerator::report_err_self_not_allowed(const sir::Param &param) {
    report_error("'self' parameter is only allowed inside structs, unions, and protos", param.ast_node);
}

void ReportGenerator::report_err_self_not_first(const sir::Param &param) {
    report_error("'self' must be the first parameter of the method", param.ast_node);
}

void ReportGenerator::report_err_case_outside_union(const sir::UnionCase &union_case) {
    report_error("'case' definition outside of a 'union' definition", union_case.ast_node);
}

void ReportGenerator::report_err_func_decl_outside_proto(const sir::FuncDecl &func_decl) {
    report_error("function declaration outside of a 'proto' definition", func_decl.ast_node);
}

void ReportGenerator::report_err_struct_overlapping_no_fields(const sir::StructDef &struct_def) {
    report_error("structs with `overlapping` layout require at least one field", struct_def.ident.ast_node);
}

void ReportGenerator::report_err_invalid_global_value(const sir::Expr &value) {
    report_error("globals cannot be initialized with this type of value", value.get_ast_node());
}

void ReportGenerator::report_err_return_missing_value(
    const sir::ReturnStmt &return_stmt,
    const sir::Expr &expected_type
) {
    report_error("'return' statement without a value (expected '$')", return_stmt.ast_node, expected_type);
}

void ReportGenerator::report_err_does_not_return(const sir::Ident &func_ident) {
    report_error("function does not return a value", func_ident.ast_node);
}

void ReportGenerator::report_err_does_not_always_return(const sir::Ident &func_ident) {
    report_error("function does not return a value in all control paths", func_ident.ast_node);
}

void ReportGenerator::report_err_pointer_to_local_escapes(const sir::Stmt &stmt, const sir::UnaryExpr &ref_expr) {
    build_error("pointer to local value escapes function", stmt.get_ast_node())
        .add_note("value is referenced here", ref_expr.ast_node)
        .report();
}

void ReportGenerator::report_err_return_ref_tmp(const sir::Expr &value) {
    report_error("cannot return reference to temporary", value.get_ast_node());
}

void ReportGenerator::report_err_return_ref_local(const sir::Expr &value, const sir::Symbol &symbol) {
    ReportBuilder builder = build_error("cannot return reference to local", value.get_ast_node());

    if (auto local = symbol.match<sir::Local>()) {
        builder.add_note("'$' is a local variable", local->name.ast_node, local->name.value);
    } else if (auto param = symbol.match<sir::Param>()) {
        builder.add_note("'$' is a non-reference parameter", param->name.ast_node, param->name.value);
    }

    builder.report();
}

void ReportGenerator::report_err_use_after_move(
    const sir::Expr &use,
    const sir::Expr &move,
    bool partial,
    bool conditional
) {
    std::string note_message = "previously moved";

    if (partial) {
        note_message += " partially";
    }

    if (conditional) {
        note_message += " in conditional branch";
    }

    note_message += " here";

    build_error("resource used after move", use.get_ast_node()).add_note(note_message, move.get_ast_node()).report();
}

void ReportGenerator::report_err_move_out_pointer(const sir::Expr &move) {
    // TODO: Different error message for references
    // TODO: Maybe change the location where the error is highlighted.
    report_error("cannot move resource out of pointer", move.get_ast_node());
}

void ReportGenerator::report_err_move_out_deinit(const sir::Expr &move) {
    // TODO: Maybe change the location where the error is highlighted.
    report_error("cannot move out of resource implementing '__deinit__'", move.get_ast_node());
}

void ReportGenerator::report_err_move_in_loop(const sir::Expr &move) {
    // TODO: Maybe highlight the loop and resource in question.
    report_error("resource moved in every iteration of a loop", move.get_ast_node());
}

void ReportGenerator::report_err_invalid_meta_field(const sir::MetaFieldExpr &meta_field_expr) {
    const sir::Ident &ident = meta_field_expr.field;
    report_error("invalid meta field '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_invalid_meta_method(const sir::MetaCallExpr &meta_call_expr) {
    const sir::Ident &ident = meta_call_expr.callee.as<sir::MetaFieldExpr>().field;
    report_error("invalid meta method '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_symbol_guarded(ASTNode *ast_node, const sir::GuardedSymbol &guarded_symbol) {
    report_error(
        "definition of '$' is guarded by different condition than usage",
        ast_node,
        guarded_symbol.variants[0].symbol.get_name()
    );
}

void ReportGenerator::report_warn_unreachable_code(const sir::Stmt &stmt) {
    report_warning("unreachable code", stmt.get_ast_node());
}

void ReportGenerator::report_err_operator_overload_not_found(
    ASTNode *ast_node,
    sir::Expr type,
    std::string_view operator_name,
    std::string_view impl_name,
    std::optional<std::string_view> second_impl_name /*= {}*/
) {
    ReportBuilder builder =
        build_error("no implementation of operator '$' for type '$'", ast_node, operator_name, type);

    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        if (second_impl_name) {
            builder.add_note(
                "implement '$' or '$' for this type to support this operator",
                struct_def->ident.ast_node,
                impl_name,
                *second_impl_name
            );
        } else {
            builder.add_note(
                "implement '$' for this type to support this operator",
                struct_def->ident.ast_node,
                impl_name
            );
        }
    }

    builder.report();
}

void ReportGenerator::report_err_unexpected_generic_arg_count(
    sir::BracketExpr &bracket_expr,
    std::vector<sir::GenericParam> &generic_params,
    sir::Ident &decl_ident,
    std::string_view decl_kind
) {
    std::string format_str;

    if (bracket_expr.rhs.size() < generic_params.size()) {
        format_str = "too few generic arguments (expected $, got $)";
    } else if (bracket_expr.rhs.size() > generic_params.size()) {
        format_str = "too many generic arguments (expected $, got $)";
    }

    build_error(format_str, bracket_expr.ast_node, generic_params.size(), bracket_expr.rhs.size())
        .add_note(
            "$ '$' defined here with the following generic parameters: $",
            decl_ident.ast_node,
            decl_kind,
            decl_ident.value,
            generic_params
        )
        .report();
}

void ReportGenerator::add_immut_sub_expr_note(ReportBuilder &builder, sir::Expr immut_sub_expr) {
    if (auto symbol_expr = immut_sub_expr.match<sir::SymbolExpr>()) {
        sir::Ident &ident = symbol_expr->symbol.get_ident();
        sir::Expr type = immut_sub_expr.get_type();

        builder.add_note("'$' is an immutable reference ('$')", ident.ast_node, ident.value, type);
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
