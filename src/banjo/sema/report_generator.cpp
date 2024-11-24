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
        return SourceLocation{find_mod_path(node), node->get_range()};
    } else {
        return SourceLocation{{}, {0, 0}};
    }
}

void ReportBuilder::report() {
    std::unordered_map<std::string_view, sir::Expr> &generic_args = analyzer.get_scope().generic_args;

    if (!generic_args.empty()) {
        for (auto &[name, value] : generic_args) {
            add_note("generic argument '$' = '$'", value.get_ast_node(), name, value);
        }
    }

    analyzer.report_manager.insert(partial_report);
}

const ModulePath &ReportBuilder::find_mod_path(ASTNode *node) {
    while (node->get_type() != AST_MODULE) {
        node = node->get_parent();
    }

    return node->as<ASTModule>()->get_path();
}

ReportGenerator::ReportGenerator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

template <typename... FormatArgs>
ReportBuilder ReportGenerator::build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    return ReportBuilder(analyzer, Report::Type::ERROR).set_message(format_str, node, format_args...);
}

template <typename... FormatArgs>
void ReportGenerator::report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    build_error(format_str, node, format_args...).report();
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
    ASSUME(unary_expr.op == sir::UnaryOp::NEG);
    std::string_view impl_name = sir::MagicMethods::look_up(unary_expr.op);
    report_err_operator_overload_not_found(unary_expr.ast_node, unary_expr.value.get_type(), "-", impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::StarExpr &star_expr) {
    std::string_view impl_name = sir::MagicMethods::look_up(sir::UnaryOp::DEREF);
    report_err_operator_overload_not_found(star_expr.ast_node, star_expr.value.get_type(), "*", impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::BracketExpr &bracket_expr) {
    report_err_operator_overload_not_found(
        bracket_expr.ast_node,
        bracket_expr.lhs.get_type(),
        "[]",
        sir::MagicMethods::OP_INDEX
    );
}

void ReportGenerator::report_err_cannot_call(const sir::Expr &expr) {
    report_error("cannot call value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_deref(const sir::Expr &expr) {
    report_error("cannot dereference value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_iter(const sir::Expr &expr) {
    report_error("cannot iterate over value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_cannot_iter_struct(const sir::Expr &expr, bool by_ref) {
    std::string note_format_str = "implement '$' for this type to support ";
    note_format_str += by_ref ? "reference iterating" : "iterating";

    sir::StructDef &struct_def = expr.get_type().as_symbol<sir::StructDef>();

    build_error("cannot iterate over value with type '$'", expr.get_ast_node(), expr.get_type())
        .add_note(note_format_str, struct_def.ident.ast_node, sir::MagicMethods::look_up_iter(by_ref))
        .report();
}

void ReportGenerator::report_err_iter_no_next(const sir::Expr &expr, const sir::FuncDef &iter_func_def, bool by_ref) {
    sir::Expr iter_type = iter_func_def.type.return_type;

    std::string note_format_str = "iterator type for ";
    note_format_str += by_ref ? "reference iterating" : "iterating";
    note_format_str += " over '$' defined here";

    build_error("iterator type '$' does not implement '$'", expr.get_ast_node(), iter_type, sir::MagicMethods::NEXT)
        .add_note(note_format_str, iter_type.get_ast_node(), expr.get_type())
        .report();
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

void ReportGenerator::report_err_unexpected_array_length(
    const sir::ArrayLiteral &array_literal,
    unsigned expected_count
) {
    std::string format_str;

    if (array_literal.values.size() < expected_count) {
        format_str = "too few elements in array literal (expected $, got $)";
    } else if (array_literal.values.size() > expected_count) {
        format_str = "too many elements in array literal (expected $, got $)";
    }

    report_error(format_str, array_literal.ast_node, expected_count, array_literal.values.size());
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

void ReportGenerator::report_err_compile_time_unknown(const sir::Expr &range) {
    report_error("value is not known at compile time", range.get_ast_node());
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
    // TODO: Maybe change the location where the error is highlighted.
    report_error("cannot move resource out of pointer", move.get_ast_node());
}

void ReportGenerator::report_err_move_out_deinit(const sir::Expr &move) {
    // TODO: Maybe change the location where the error is highlighted.
    report_error("cannot move out of resource implementing '__deinit__'", move.get_ast_node());
}

void ReportGenerator::report_err_operator_overload_not_found(
    ASTNode *ast_node,
    sir::Expr type,
    std::string_view operator_name,
    std::string_view impl_name
) {
    ReportBuilder builder =
        build_error("no implementation of operator '$' for type '$'", ast_node, operator_name, type);

    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        builder.add_note("implement '$' for this type to support this operator", struct_def->ident.ast_node, impl_name);
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

} // namespace sema

} // namespace lang

} // namespace banjo
